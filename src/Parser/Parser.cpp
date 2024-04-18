//
// Created by Albert Varaksin on 03/07/2020.
//
#include "Parser.hpp"
#include "Ast/Ast.hpp"
#include "Diag/DiagnosticEngine.hpp"
#include "Driver/CompileOptions.hpp"
#include "Driver/Context.hpp"
#include "Lexer/Lexer.hpp"
#include "Lexer/Token.hpp"
#include "Symbol/SymbolTable.hpp"
#include "Type/Type.hpp"
#include <llvm/ADT/TypeSwitch.h>
using namespace lbc;
using namespace flags::operators;

Parser::Parser(Context& context, Lexer& lexer, bool isMain, SymbolTable* symbolTable)
: m_context{ context },
  m_lexer{ lexer },
  m_isMain{ isMain },
  m_symbolTable{ symbolTable },
  m_language{ CallingConv::Default },
  m_diag{ context.getDiag() },
  m_scope{ Scope::Root } {
    advance();
}

void Parser::reset() {
    m_scope = Scope::Root;
    m_exprFlags = {};
    m_endLoc = {};
    m_token = {};
    advance();
    m_endLoc = m_token.range().Start;
}

/**
 * Module
 *   = StmtList
 *   .
 */
Result<AstModule*> Parser::parse() {
    auto stmts = stmtList();
    TRY(stmts)

    return m_context.create<AstModule>(
        m_lexer.getFileId(),
        stmts->range,
        m_isMain,
        std::move(m_imports),
        stmts);
}

//----------------------------------------
// Statements
//----------------------------------------

/**
 * StmtList
 *   = { Statement }
 *   .
 */
Result<AstStmtList*> Parser::stmtList() {
    constexpr auto isNonTerminator = [](const Token& token) {
        switch (token.getKind()) {
        case TokenKind::End:
        case TokenKind::Else:
        case TokenKind::Next:
        case TokenKind::Loop:
        case TokenKind::EndOfFile:
            return false;
        default:
            return true;
        }
    };

    auto start = m_token.range().Start;
    std::vector<AstDecl*> decls;
    std::vector<AstFuncStmt*> funcs;
    std::vector<AstStmt*> stms;

    while (isNonTerminator(m_token)) {
        auto stmt = statement();
        TRY(stmt)

        llvm::TypeSwitch<AstStmt*>(stmt)
            .Case([&](AstFuncStmt* node) {
                funcs.emplace_back(node);
                decls.emplace_back(node->decl);
            })
            .Case([&](AstDecl* node) {
                decls.emplace_back(node);
                stms.emplace_back(node);
            })
            .Case([&](AstImport* import) {
                m_imports.emplace_back(import);
            })
            .Case([&](AstExtern* ext) {
                for (auto* eStmt : ext->stmts) {
                    if (auto* decl = llvm::dyn_cast<AstDecl>(eStmt)) {
                        decl->local = false;
                        decls.emplace_back(decl);
                        stms.emplace_back(decl);
                    } else if (auto* func = llvm::dyn_cast<AstFuncStmt>(eStmt)) {
                        decls.emplace_back(func->decl);
                        funcs.emplace_back(func);
                    } else {
                        fatalError("Unknown declaration");
                    }
                }
            })
            .Default([&](auto* node) {
                stms.emplace_back(node);
            });

        TRY(consume(TokenKind::EndOfStmt))
    }

    return m_context.create<AstStmtList>(
        llvm::SMRange{ start, m_endLoc },
        std::move(decls),
        std::move(funcs),
        std::move(stms));
}

/**
 * Statement
 *   = Declaration
 *   | IMPORT
 *   | IfStmt
 *   | ForStmt
 *   | DoLoopStmt
 *   | RETURN
 *   | EXIT
 *   | CONTINUE
 *   | Expression
 *   .
 */
Result<AstStmt*> Parser::statement() {
    auto decl = declaration();
    TRY(decl)

    if (decl != nullptr) {
        return decl;
    }

    if (m_scope == Scope::Root) {
        if (m_token.is(TokenKind::Import)) {
            return kwImport();
        }
        if (m_token.is(TokenKind::Extern)) {
            return kwExtern();
        }
        if (!m_isMain) {
            return makeError(Diag::notAllowedTopLevelStatement);
        }
    }

    switch (m_token.getKind()) {
    case TokenKind::Return:
        return kwReturn();
    case TokenKind::If:
        return kwIf();
    case TokenKind::For:
        return kwFor();
    case TokenKind::Do:
        return kwDo();
    case TokenKind::Continue:
        return kwContinue();
    case TokenKind::Exit:
        return kwExit();
    default:
        break;
    }

    auto expr = expression(ExprFlags::useAssign | ExprFlags::callWithoutParens);
    TRY(expr)
    return m_context.create<AstExprStmt>(expr->range, expr);
}

/**
 * IMPORT
 *   = "IMPORT" id
 *   .
 */
Result<AstImport*> Parser::kwImport() {
    // assume m_token == Import
    advance();

    TRY(expect(TokenKind::Identifier))

    auto import = m_token.lexeme();
    auto range = m_token.range();
    advance();

    // Imported file
    auto source = m_context.getOptions().getCompilerDir() / "lib" / (import + ".bas").str();
    if (!m_context.import(source.string())) {
        return m_context.create<AstImport>(llvm::SMRange{ range.Start, m_endLoc }, import);
    }
    if (!fs::exists(source)) {
        return m_diag.makeError(Diag::moduleNotFound, range, import);
    }

    // Load import into Source Mgr
    std::string included;
    auto ID = m_context.getSourceMrg().AddIncludeFile(
        source.string(),
        range.Start,
        included);
    if (ID == ~0U) {
        return m_diag.makeError(Diag::failedToLoadModule, range, source.string());
    }

    // parse the module
    Lexer lexer{ m_context, ID };
    auto module = Parser(m_context, lexer, false).parse();
    TRY(module)

    return m_context.create<AstImport>(
        llvm::SMRange{ range.Start, m_endLoc },
        import,
        module);
}

/**
 * Extern
 *   = [ LanguageStringLiteral ]
 *   ( Statement
 *   | StatementList "END" "EXTERN"
 *   .
 */
Result<AstExtern*> Parser::kwExtern() {
    // assume m_token == Extern
    auto start = m_token.range().Start;
    advance();
    RESTORE_ON_EXIT(m_language);

    if (m_token.is(TokenKind::StringLiteral)) {
        auto str = m_token.getStringValue().upper();
        if (str == "C") {
            m_language = CallingConv::C;
        } else if (str == "DEFAULT") {
            m_language = CallingConv::Default;
        } else {
            return makeError(Diag::unsupportedExternLanguage, str);
        }
        advance();
    }

    std::vector<AstStmt*> stmts{};

    if (accept(TokenKind::EndOfStmt)) {
        while (m_token.isNot(TokenKind::End)) {
            auto decl = declaration();
            TRY(decl)
            if (decl == nullptr) {
                return makeError(Diag::onlyDeclarationsInExtern);
            }
            stmts.emplace_back(decl);
            TRY(consume(TokenKind::EndOfStmt))
        }
        TRY(consume(TokenKind::End))
        TRY(consume(TokenKind::Extern))
    } else {
        auto decl = declaration();
        TRY(decl)
        stmts.emplace_back(decl);
    }

    return m_context.create<AstExtern>(
        llvm::SMRange{ start, m_endLoc },
        m_language,
        std::move(stmts));
}

/**
 * Declaration
 *   = [
 *     [ AttributeList ]
 *     ( VAR
 *     | DECLARE
 *     | FUNCTION
 *     | SUB
 *     )
 *   ]
 *   .
 */
Result<AstStmt*> Parser::declaration() {
    auto attribs = attributeList();
    TRY(attribs)

    switch (m_token.getKind()) {
    case TokenKind::Dim:
        return kwDim(attribs);
    case TokenKind::Declare:
        return kwDeclare(attribs);
    case TokenKind::Function:
    case TokenKind::Sub:
        return kwFunction(attribs);
    case TokenKind::Type:
        return kwType(attribs);
    default:
        break;
    }

    if (attribs != nullptr) {
        return makeError(Diag::expectedDeclarationAfterAttribute, m_token.description());
    }
    return nullptr;
}

//----------------------------------------
// Attributes
//----------------------------------------

/**
 *  AttributeList = [ '[' Attribute { ','  Attribute } ']' ].
 */
Result<AstAttributeList*> Parser::attributeList() {
    if (m_token.isNot(TokenKind::BracketOpen)) {
        return nullptr;
    }

    auto start = m_token.range().Start;
    advance();

    std::vector<AstAttribute*> attribs;

    do {
        auto attrib = attribute();
        TRY(attrib)

        attribs.emplace_back(attrib);
    } while (accept(TokenKind::Comma));

    TRY(consume(TokenKind::BracketClose))

    return m_context.create<AstAttributeList>(
        llvm::SMRange{ start, m_endLoc },
        std::move(attribs));
}

/**
 * Attribute
 *   = IdentExpr [ AttributeArgList ]
 *   .
 */
Result<AstAttribute*> Parser::attribute() {
    auto start = m_token.range().Start;

    auto id = identifier();
    TRY(id)

    Result<AstExprList*> args{};
    if (m_token.isOneOf(TokenKind::Assign, TokenKind::ParenOpen)) {
        args = attributeArgList();
        TRY(args)
    }

    return m_context.create<AstAttribute>(
        llvm::SMRange{ start, m_endLoc },
        id,
        args);
}

/**
 * AttributeArgList
 *   = "=" Literal
 *   | "(" [ Literal { "," Literal } ] ")"
 *   .
 */
Result<AstExprList*> Parser::attributeArgList() {
    auto start = m_token.range().Start;
    std::vector<AstExpr*> args;

    if (accept(TokenKind::Assign)) {
        auto lit = literal();
        TRY(lit)
        args.emplace_back(lit);
    } else if (accept(TokenKind::ParenOpen)) {
        while (!m_token.isOneOf(TokenKind::EndOfFile, TokenKind::ParenClose)) {
            auto lit = literal();
            TRY(lit)
            args.emplace_back(lit);
            if (!accept(TokenKind::Comma)) {
                break;
            }
        }
        TRY(consume(TokenKind::ParenClose))
    }

    return m_context.create<AstExprList>(
        llvm::SMRange{ start, m_endLoc },
        std::move(args));
}

//----------------------------------------
// VAR
//----------------------------------------

/**
 * Dim
 *   = "DIM" identifier
 *   ( "=" Expression
 *   | "AS" TypeExpr [ "=" Expression ]
 *   )
 *   .
 */
Result<AstVarDecl*> Parser::kwDim(AstAttributeList* attribs) {
    // assume m_token == VAR
    auto start = attribs != nullptr ? attribs->range.Start : m_token.range().Start;
    advance();

    TRY(expect(TokenKind::Identifier))
    auto id = m_token.getStringValue();
    auto token = m_token;
    advance();

    Result<AstTypeExpr*> type{};
    Result<AstExpr*> expr{};

    if (accept(TokenKind::As)) {
        type = typeExpr();
        TRY(type)

        if (accept(TokenKind::Assign)) {
            expr = expression();
            TRY(expr)
        }
    } else {
        TRY(consume(TokenKind::Assign))
        expr = expression();
        TRY(expr)
    }

    return m_context.create<AstVarDecl>(
        llvm::SMRange{ start, m_endLoc },
        id,
        token,
        m_language,
        attribs,
        type,
        expr);
}

//----------------------------------------
// DECLARE
//----------------------------------------

/**
 * DECLARE
 *   = "DECLARE" FuncSignature
 *   .
 */
Result<AstFuncDecl*> Parser::kwDeclare(AstAttributeList* attribs) {
    // assume m_token == DECLARE
    if (m_scope != Scope::Root) {
        return makeError(Diag::unexpectedNestedDeclaration, m_token.description());
    }
    auto start = attribs != nullptr ? attribs->range.Start : m_token.range().Start;
    advance();

    return funcSignature(start, attribs, FuncFlags::isDeclaration);
}

/**
 * FuncSignature
 *     = "FUNCTION" id [ "(" [ FuncParamList ] ")" ] "AS" TypeExpr
 *     | "SUB" id [ "(" FuncParamList ")" ]
 *     .
 */
Result<AstFuncDecl*> Parser::funcSignature(llvm::SMLoc start, AstAttributeList* attribs, FuncFlags funcFlags) {
    bool const isFunc = accept(TokenKind::Function);
    if (!isFunc) {
        TRY(consume(TokenKind::Sub))
    }

    llvm::StringRef id;
    Token token;
    if (flags::has(funcFlags, FuncFlags::isAnonymous)) {
        id = "";
    } else {
        TRY(expect(TokenKind::Identifier))
        id = m_token.getStringValue();
        token = m_token;
        advance();
    }

    bool isVariadic = false;
    Result<AstFuncParamList*> params{};
    if (accept(TokenKind::ParenOpen)) {
        params = funcParamList(isVariadic, flags::has(funcFlags, FuncFlags::isAnonymous));
        TRY(params)
        TRY(consume(TokenKind::ParenClose))
    }

    Result<AstTypeExpr*> ret{};
    if (isFunc) {
        TRY(consume(TokenKind::As))
        ret = typeExpr();
        TRY(ret)
    }

    return m_context.create<AstFuncDecl>(
        llvm::SMRange{ start, m_endLoc },
        id,
        token,
        m_language,
        attribs,
        params,
        isVariadic,
        ret,
        !flags::has(funcFlags, FuncFlags::isDeclaration));
}

/**
 * FuncParamList
 *   = FuncParam { "," FuncParam } [ "," "..." ]
 *   | "..."
 *   .
 */
Result<AstFuncParamList*> Parser::funcParamList(bool& isVariadic, bool isAnonymous) {
    auto start = m_token.range().Start;
    std::vector<AstFuncParamDecl*> params;
    while (!m_token.isOneOf(TokenKind::EndOfFile, TokenKind::ParenClose)) {
        if (accept(TokenKind::Ellipsis)) {
            isVariadic = true;
            if (m_token.is(TokenKind::Comma)) {
                return makeError(Diag::variadicArgumentNotLast);
            }
            break;
        }
        auto param = funcParam(isAnonymous);
        TRY(param)

        params.push_back(param);
        if (!accept(TokenKind::Comma)) {
            break;
        }
    }

    return m_context.create<AstFuncParamList>(
        llvm::SMRange{ start, m_endLoc },
        std::move(params));
}

/**
 * FuncParam
 *  = id "AS" TypeExpr
 *  | TypeExpr        // if isAnonymous
 *  .
 */
Result<AstFuncParamDecl*> Parser::funcParam(bool isAnonymous) {
    auto start = m_token.range().Start;

    llvm::StringRef id;
    Token token;
    if (isAnonymous) {
        if (m_token.is(TokenKind::Identifier)) {
            Token next;
            m_lexer.peek(next);
            if (next.is(TokenKind::As)) {
                id = m_token.getStringValue();
                token = m_token;
                advance();
                advance();
            }
        } else {
            id = "";
        }
    } else {
        TRY(expect(TokenKind::Identifier))
        id = m_token.getStringValue();
        token = m_token;
        advance();
        TRY(consume(TokenKind::As))
    }

    auto type = typeExpr();
    TRY(type)

    return m_context.create<AstFuncParamDecl>(
        llvm::SMRange{ start, m_endLoc },
        id,
        token,
        m_language,
        nullptr,
        type);
}

//----------------------------------------
// TYPE
//----------------------------------------

/**
 * TYPE
 *   = "TYPE" id
 *   ( UDT
 *   | TypeAlias
 *   )
 *   .
 */
Result<AstDecl*> Parser::kwType(AstAttributeList* attribs) {
    // assume m_token == TYPE
    auto start = m_token.range().Start;
    advance();

    TRY(expect(TokenKind::Identifier))
    auto id = m_token.getStringValue();
    auto token = m_token;
    advance();

    if (accept(TokenKind::EndOfStmt)) {
        return udt(id, token, start, attribs);
    }

    if (accept(TokenKind::As)) {
        return alias(id, token, start, attribs);
    }

    return makeError(Diag::unexpectedToken, "'=' or end of statement", m_token.description());
}

/**
 *  alias
 *    = TypeExpr
 *    .
 */
Result<AstTypeAlias*> Parser::alias(llvm::StringRef id, Token token, llvm::SMLoc start, AstAttributeList* attribs) {
    auto type = typeExpr();
    TRY(type)

    return m_context.create<AstTypeAlias>(
        llvm::SMRange{ start, m_endLoc },
        id,
        token,
        m_language,
        attribs,
        type);
}

/**
 * UDT
 *   = EoS
 *     udtDeclList
 *     "END" "TYPE"
 *   .
 */
Result<AstUdtDecl*> Parser::udt(llvm::StringRef id, Token token, llvm::SMLoc start, AstAttributeList* attribs) {
    // assume m_token == declaration || "end"
    auto decls = udtDeclList();
    TRY(decls)

    TRY(consume(TokenKind::End))
    TRY(consume(TokenKind::Type))

    return m_context.create<AstUdtDecl>(
        llvm::SMRange{ start, m_endLoc },
        id,
        token,
        m_language,
        attribs,
        decls);
}

/**
 * udtDeclList
 *   = { [ AttributeList ] udtMember EoS }
 *   .
 */
Result<AstDeclList*> Parser::udtDeclList() {
    auto start = m_token.range().Start;
    std::vector<AstDecl*> decls;

    while (true) {
        auto attribs = attributeList();
        TRY(attribs)

        if (attribs != nullptr) {
            TRY(expect(TokenKind::Identifier))
        } else if (m_token.isNot(TokenKind::Identifier)) {
            break;
        }

        auto member = udtMember(attribs);
        TRY(member)

        decls.emplace_back(member);
        TRY(consume(TokenKind::EndOfStmt))
    }

    return m_context.create<AstDeclList>(
        llvm::SMRange{ start, m_endLoc },
        std::move(decls));
}

/**
 * udtMember
 *   = id "AS" TypeExpr
 *   .
 */
Result<AstDecl*> Parser::udtMember(AstAttributeList* attribs) {
    // assume m_token == Identifier
    auto start = m_token.range().Start;
    auto id = m_token.getStringValue();
    auto token = m_token;
    advance();

    TRY(consume(TokenKind::As))

    auto type = typeExpr();
    TRY(type)

    return m_context.create<AstVarDecl>(
        llvm::SMRange{ start, m_endLoc },
        id,
        token,
        m_language,
        attribs,
        type,
        nullptr);
}

//----------------------------------------
// Call
//----------------------------------------

/**
 *  FUNCTION = funcSignature <EoS>
 *             stmtList
 *             "END" ("FUNCTION" | "SUB")
 */
Result<AstFuncStmt*> Parser::kwFunction(AstAttributeList* attribs) {
    if (m_scope != Scope::Root) {
        return makeError(Diag::unexpectedNestedDeclaration, m_token.description());
    }

    bool const isFunction = m_token.is(TokenKind::Function);
    auto start = attribs != nullptr ? attribs->range.Start : m_token.range().Start;
    auto decl = funcSignature(start, attribs, {});
    TRY(decl)

    RESTORE_ON_EXIT(m_scope);
    m_scope = Scope::Function;
    Result<AstStmtList*> stmts{};

    if (accept(TokenKind::LambdaBody)) {
        Result<AstStmt*> stmt{};
        if (isFunction) {
            auto expr = expression();
            TRY(expr)

            stmt = m_context.create<AstReturnStmt>(
                llvm::SMRange{ start, m_endLoc },
                expr);
        } else {
            stmt = statement();
            TRY(stmt)
        }
        stmts = m_context.create<AstStmtList>(
            llvm::SMRange{ start, m_endLoc },
            std::vector<AstDecl*>{}, // TODO: Fix. stmt could be a declaration!
            std::vector<AstFuncStmt*>{},
            std::vector<AstStmt*>{ stmt });
    } else {
        TRY(consume(TokenKind::EndOfStmt))
        stmts = stmtList();
        TRY(stmts)

        TRY(consume(TokenKind::End))
        if (isFunction) {
            TRY(consume(TokenKind::Function))
        } else {
            TRY(consume(TokenKind::Sub))
        }
    }

    return m_context.create<AstFuncStmt>(
        llvm::SMRange{ start, m_endLoc },
        decl,
        stmts);
}

/**
 * RETURN = "RETURN" [ expression ] .
 */
Result<AstStmt*> Parser::kwReturn() {
    // assume m_token == RETURN
    if (m_scope == Scope::Root && !m_isMain) {
        return makeError(Diag::unexpectedReturn);
    }
    auto start = m_token.range().Start;
    advance();

    Result<AstExpr*> expr{};
    if (m_token.isNot(TokenKind::EndOfStmt)) {
        expr = expression();
        TRY(expr)
    }

    return m_context.create<AstReturnStmt>(
        llvm::SMRange{ start, m_endLoc },
        expr);
}

//----------------------------------------
// If statement
//----------------------------------------

/**
 * IF
 *   = IfBlock
 *   { ELSE IF IfBlock }
 *   [ ELSE ThenBlock ]
 *   "END" "IF"
 *   .
 */
Result<AstIfStmt*> Parser::kwIf() {
    // assume m_token == IF
    auto start = m_token.range().Start;
    advance();

    std::vector<AstIfStmtBlock*> blocks;
    auto block = ifBlock();
    TRY(block)

    blocks.emplace_back(block);

    if (m_token.is(TokenKind::EndOfStmt)) {
        Token next;
        m_lexer.peek(next);
        if (next.getKind() == TokenKind::Else) {
            advance();
        }
    }

    while (accept(TokenKind::Else)) {
        if (accept(TokenKind::If)) {
            auto if_ = ifBlock();
            TRY(if_)
            blocks.emplace_back(if_);
        } else {
            auto else_ = thenBlock({}, nullptr);
            TRY(else_)
            blocks.emplace_back(else_);
        }

        if (m_token.is(TokenKind::EndOfStmt)) {
            Token next;
            m_lexer.peek(next);
            if (next.getKind() == TokenKind::Else) {
                advance();
            }
        }
    }

    if (blocks.back()->stmt->kind == AstKind::StmtList) {
        TRY(consume(TokenKind::End))
        TRY(consume(TokenKind::If))
    }

    return m_context.create<AstIfStmt>(
        llvm::SMRange{ start, m_endLoc },
        std::move(blocks));
}

/**
 * IfBlock
 *   = [ VAR { "," VAR } "," ] Expression "THEN" ThenBlock
 *   .
 */
Result<AstIfStmtBlock*> Parser::ifBlock() {
    std::vector<AstVarDecl*> decls;
    while (m_token.is(TokenKind::Dim)) {
        auto var = kwDim(nullptr);
        TRY(var)

        decls.emplace_back(var);
        TRY(consume(TokenKind::Comma))
    }

    auto expr = expression(ExprFlags::commaAsAnd);
    TRY(expr)

    TRY(consume(TokenKind::Then))

    return thenBlock(std::move(decls), expr);
}

/**
 * ThenBlock
 *   =
 *   ( EoS StmtList
 *   | Statement
 *   )
 *   .
 */
Result<AstIfStmtBlock*> Parser::thenBlock(std::vector<AstVarDecl*> decls, AstExpr* expr) {
    Result<AstStmt*> stmt{};
    if (accept(TokenKind::EndOfStmt)) {
        stmt = stmtList();
        TRY(stmt)
    } else {
        stmt = statement();
        TRY(stmt)
    }

    return m_context.create<AstIfStmtBlock>(
        std::move(decls),
        nullptr,
        expr,
        stmt);
}

//----------------------------------------
// FOR statement
//----------------------------------------

/**
 * FOR
 *   = "FOR" [ VAR { "," VAR } "," ]
 *     id [ "AS" TypeExpr ] "=" Expression "TO" Expression [ "STEP" expression ]
 *   ( "=>" Statement
 *   | <EoS> StatementList
 *     "NEXT" [ id ]
 *   )
 *   .
 */
Result<AstForStmt*> Parser::kwFor() {
    // assume m_token == FOR
    auto start = m_token.range().Start;
    advance();

    std::vector<AstVarDecl*> decls;

    // [ VAR { "," VAR } "," ]
    while (m_token.is(TokenKind::Dim)) {
        auto var = kwDim(nullptr);
        TRY(var)

        decls.emplace_back(var);
        TRY(consume(TokenKind::Comma))
    }

    // id [ "AS" TypeExpr ] "=" Expression
    auto idStart = m_token.range().Start;
    TRY(expect(TokenKind::Identifier))
    auto id = m_token.getStringValue();
    auto token = m_token;
    advance();

    Result<AstTypeExpr*> type{};
    if (accept(TokenKind::As)) {
        type = typeExpr();
        TRY(type)
    }

    TRY(consume(TokenKind::Assign))

    auto expr = expression();
    TRY(expr)

    auto* iterator = m_context.create<AstVarDecl>(
        llvm::SMRange{ idStart, m_endLoc },
        id,
        token,
        m_language,
        nullptr,
        type,
        expr);

    // "TO" Expression [ "STEP" expression ]
    TRY(consume(TokenKind::To))

    auto limit = expression();
    TRY(limit)

    Result<AstExpr*> step{};
    if (accept(TokenKind::Step)) {
        step = expression();
        TRY(step)
    }

    // "=>" statement ?
    Result<AstStmt*> stmt{};
    llvm::StringRef next;
    if (accept(TokenKind::LambdaBody)) {
        stmt = statement();
        TRY(stmt)
    } else {
        TRY(consume(TokenKind::EndOfStmt))
        stmt = stmtList();
        TRY(stmt)

        TRY(consume(TokenKind::Next))

        if (m_token.is(TokenKind::Identifier)) {
            next = m_token.getStringValue();
            advance();
        }
    }

    return m_context.create<AstForStmt>(
        llvm::SMRange{ start, m_endLoc },
        std::move(decls),
        iterator,
        limit,
        step,
        stmt,
        next);
}

//----------------------------------------
// DO ... LOOP statement
//----------------------------------------

/**
 * DO = "DO" [ VAR { "," VAR } ]
 *    ( EndOfStmt StmtList "LOOP" [ LoopCondition ]
 *    | [ LoopCondition ] ( EoS StmtList "LOOP" | "=>" Statement )
 *    )
 *    .
 * LoopCondition
 *   = ("UNTIL" | "WHILE") expression
 *   .
 */
Result<AstDoLoopStmt*> Parser::kwDo() {
    // assume m_token == DO
    auto start = m_token.range().Start;
    advance();

    auto condition = AstDoLoopStmt::Condition::None;
    Result<AstStmt*> stmt{};
    Result<AstExpr*> expr{};
    std::vector<AstVarDecl*> decls;

    // [ VAR { "," VAR } ]
    auto acceptComma = false;
    while (m_token.is(TokenKind::Dim) || (acceptComma && accept(TokenKind::Comma))) {
        acceptComma = true;
        auto var = kwDim(nullptr);
        TRY(var)

        decls.emplace_back(var);
    }

    // ( EoS StmtList "LOOP" [ Condition ]
    if (accept(TokenKind::EndOfStmt)) {
        stmt = stmtList();
        TRY(stmt)

        TRY(consume(TokenKind::Loop))

        // [ Condition ]
        if (accept(TokenKind::Until)) {
            condition = AstDoLoopStmt::Condition::PostUntil;
            expr = expression(ExprFlags::commaAsAnd);
            TRY(expr)
        } else if (accept(TokenKind::While)) {
            condition = AstDoLoopStmt::Condition::PostWhile;
            expr = expression(ExprFlags::commaAsAnd);
            TRY(expr)
        }
    } else {
        // [ Condition ]
        if (accept(TokenKind::Until)) {
            condition = AstDoLoopStmt::Condition::PreUntil;
            expr = expression(ExprFlags::commaAsAnd);
            TRY(expr)
        } else if (accept(TokenKind::While)) {
            condition = AstDoLoopStmt::Condition::PreWhile;
            expr = expression(ExprFlags::commaAsAnd);
            TRY(expr)
        }

        // EoS StmtList "LOOP"
        if (accept(TokenKind::EndOfStmt)) {
            stmt = stmtList();
            TRY(stmt)

            TRY(consume(TokenKind::Loop))
        }
        // "=>" Statement
        else {
            TRY(consume(TokenKind::LambdaBody))
            stmt = statement();
            TRY(stmt)
        }
    }

    return m_context.create<AstDoLoopStmt>(
        llvm::SMRange{ start, m_endLoc },
        std::move(decls),
        condition,
        expr,
        stmt);
}

//----------------------------------------
// Branching
//----------------------------------------

/**
 * CONTINUE
 *   = "CONTINUE" { "FOR" }
 *   .
 */
Result<AstContinuationStmt*> Parser::kwContinue() {
    // assume m_token == CONTINUE
    auto start = m_token.range().Start;
    advance();

    std::vector<ControlFlowStatement> returnControl;

    while (true) {
        switch (m_token.getKind()) {
        case TokenKind::For:
            advance();
            returnControl.emplace_back(ControlFlowStatement::For);
            continue;
        case TokenKind::Do:
            advance();
            returnControl.emplace_back(ControlFlowStatement::Do);
            continue;
        default:
            break;
        }
        break;
    }

    return m_context.create<AstContinuationStmt>(
        llvm::SMRange{ start, m_endLoc },
        AstContinuationStmt::Action::Continue,
        std::move(returnControl));
}

/**
 * EXIT
 *   = "EXIT" { "FOR" }
 *   .
 */
Result<AstContinuationStmt*> Parser::kwExit() {
    // assume m_token == EXIT
    auto start = m_token.range().Start;
    advance();

    std::vector<ControlFlowStatement> returnControl;

    while (true) {
        switch (m_token.getKind()) {
        case TokenKind::For:
            advance();
            returnControl.emplace_back(ControlFlowStatement::For);
            continue;
        case TokenKind::Do:
            advance();
            returnControl.emplace_back(ControlFlowStatement::Do);
            continue;
        default:
            break;
        }
        break;
    }

    return m_context.create<AstContinuationStmt>(
        llvm::SMRange{ start, m_endLoc },
        AstContinuationStmt::Action::Exit,
        std::move(returnControl));
}

//----------------------------------------
// Types
//----------------------------------------

/**
 * TypeExpr = ( identExpr | Any ) { "PTR" }
 *          | "SUB" "(" { FuncParamList } ")" "PTR" { "PTR" }
 *          | "FUNCTION" "(" { FuncParamList } ")" "AS" TypeExpr "PTR" { "PTR" }
 *          | "(" TypeExpr ")"
 *          | TypeOf
 *          .
 */
Result<AstTypeExpr*> Parser::typeExpr() {
    auto start = m_token.range().Start;
    bool const parenthesized = accept(TokenKind::ParenOpen);
    bool mustBePtr = false;

    AstTypeExpr::TypeExpr expr;
    if (m_token.isOneOf(TokenKind::Sub, TokenKind::Function)) {
        auto signature = funcSignature(start, nullptr, FuncFlags::isAnonymous);
        TRY(signature)
        expr = signature;
        mustBePtr = true;
    } else if (m_token.is(TokenKind::Any) || m_token.isTypeKeyword()) {
        expr = m_token.getKind();
        advance();
    } else if (m_token.is(TokenKind::TypeOf)) {
        auto typeOf = kwTypeOf();
        TRY(typeOf)
        expr = typeOf;
    } else {
        auto ident = identifier();
        TRY(ident)

        if (m_symbolTable != nullptr) {
            auto* symbol = m_symbolTable->find(ident->name);
            if (symbol == nullptr || symbol->valueFlags().kind != ValueFlags::Kind::type) {
                return ResultError{};
            }
        }
        expr = ident;
    }

    if (parenthesized) {
        TRY(consume(TokenKind::ParenClose))
    }

    auto deref = 0;
    while (accept(TokenKind::Ptr)) {
        deref++;
    }

    if (mustBePtr && deref == 0) {
        return ResultError{};
    }

    return m_context.create<AstTypeExpr>(
        llvm::SMRange{ start, m_endLoc },
        expr,
        deref);
}

/**
 * TypeOf = "TYPEOF" "(" (Expr | TypeExpr) ")"
 *        .
 */
Result<AstTypeOf*> Parser::kwTypeOf() {
    // assume m_token == "TYPEOF"
    auto start = m_token.range().Start;
    advance();

    TRY(consume(TokenKind::ParenOpen))
    llvm::SMRange exprRange = m_token.range();
    int parens = 1;
    while (true) {
        if (m_token.isOneOf(TokenKind::EndOfStmt, TokenKind::EndOfFile, TokenKind::Invalid)) {
            return makeError(Diag::unexpectedToken, "type expression", m_token.description());
        }
        if (m_token.is(TokenKind::ParenClose)) {
            parens--;
            if (parens == 0) {
                if (exprRange.End == m_token.range().End) {
                    return makeError(Diag::unexpectedToken, "type expression", m_token.description());
                }
                advance();
                break;
            }
            if (parens < 0) {
                return makeError(Diag::unexpectedToken, "type expression", m_token.description());
            }
        } else if (m_token.is(TokenKind::ParenOpen)) {
            parens++;
        }
        exprRange.End = m_token.range().End;
        advance();
    }
    return m_context.create<AstTypeOf>(llvm::SMRange{ start, m_endLoc }, exprRange);
}

//----------------------------------------
// Expressions
//----------------------------------------

/**
 * expression = factor { <Binary Op> expression }
 *            . [ ArgumentList ]
 */
Result<AstExpr*> Parser::expression(ExprFlags flags) {
    static constexpr auto allowCallWithToken = [](const Token& token) {
        switch (token.getKind()) {
        case TokenKind::Multiply:
        case TokenKind::Minus:
            return true;
        default:
            return !token.isBinary();
        }
    };

    RESTORE_ON_EXIT(m_exprFlags);
    m_exprFlags = flags;
    auto expr = factor();
    TRY(expr)

    fixExprOperators();

    // expr op
    if (m_token.isOperator()) {
        expr = expression(expr, 1);
        TRY(expr)
    }

    // print "hello"
    if (flags::has(flags, ExprFlags::callWithoutParens) && llvm::isa<AstIdentExpr>(*expr) && allowCallWithToken(m_token)) {
        auto start = expr->range.Start;
        auto args = expressionList();
        TRY(args)

        expr = m_context.create<AstCallExpr>(
            llvm::SMRange{ start, m_endLoc },
            expr,
            args);
    }

    return expr;
}

/**
 * Recursievly climb operator precedence
 * https://en.wikipedia.org/wiki/Operator-precedence_parser#Precedence_climbing_method
 */
Result<AstExpr*> Parser::expression(AstExpr* lhs, int precedence) {
    while (m_token.getPrecedence() >= precedence) {
        auto current = m_token.getPrecedence();
        auto kind = m_token.getKind();
        if (!m_token.isBinary()) {
            return makeError(Diag::unexpectedToken, "binary operator", m_token.description());
        }
        advance();

        auto rhs = factor();
        TRY(rhs)

        fixExprOperators();

        while (m_token.getPrecedence() > current || (m_token.isRightToLeft() && m_token.getPrecedence() == current)) {
            rhs = expression(rhs, m_token.getPrecedence());
            TRY(rhs)
        }

        auto start = lhs->range.Start;
        auto bin = binary({ start, m_endLoc }, kind, lhs, rhs);
        TRY(bin)
        lhs = bin;
    }
    return lhs;
}

void Parser::fixExprOperators() {
    if (m_token.is(TokenKind::Assign)) {
        if (flags::has(m_exprFlags, ExprFlags::useAssign)) {
            flags::unset(m_exprFlags, ExprFlags::useAssign);
        } else {
            m_token.setKind(TokenKind::Equal);
        }
    } else if (m_token.is(TokenKind::Comma) && flags::has(m_exprFlags, ExprFlags::commaAsAnd)) {
        m_token.setKind(TokenKind::CommaAnd);
    }
}

/**
 * factor = primary { <Right Unary Op> | "AS" TypeExpr } .
 */
Result<AstExpr*> Parser::factor() {
    auto start = m_token.range().Start;
    auto expr = primary();
    TRY(expr)

    while (true) {
        // <Right Unary Op>
        if (m_token.isUnary() && m_token.isRightToLeft()) {
            auto kind = m_token.getKind();
            advance();
            expr = unary({ start, m_endLoc }, kind, expr);
            TRY(expr)

            continue;
        }

        if (accept(TokenKind::ParenOpen)) {
            auto args = expressionList();
            TRY(args)

            TRY(consume(TokenKind::ParenClose))
            expr = m_context.create<AstCallExpr>(
                llvm::SMRange{ start, m_endLoc },
                expr,
                args);
            continue;
        }

        // "AS" TypeExpr
        if (accept(TokenKind::As)) {
            auto type = typeExpr();
            TRY(type)

            auto* cast = m_context.create<AstCastExpr>(
                llvm::SMRange{ start, m_endLoc },
                expr,
                type,
                false);
            expr = cast;
            continue;
        }
        break;
    }
    return expr;
}

/**
 * primary = literal
 *         | CallExpr
 *         | identifier [ "(" params ") ]
 *         | "(" expression ")"
 *         | <Left Unary Op> [ factor { <Binary Op> expression } ]
 *         | IfExpr
 *        .
 */
Result<AstExpr*> Parser::primary() {
    if (m_token.isLiteral()) {
        return literal();
    }

    if (m_token.is(TokenKind::Identifier)) {
        return identifier();
    }

    if (accept(TokenKind::ParenOpen)) {
        auto expr = expression();
        TRY(expr)

        TRY(consume(TokenKind::ParenClose))
        return expr;
    }

    if (m_token.is(TokenKind::If)) {
        return ifExpr();
    }

    replace(TokenKind::Minus, TokenKind::Negate);
    replace(TokenKind::Multiply, TokenKind::Dereference);
    if (m_token.isUnary() && m_token.isLeftToRight()) {
        auto start = m_token.range().Start;
        auto prec = m_token.getPrecedence();
        auto kind = m_token.getKind();
        advance();

        auto fact = factor();
        TRY(fact)

        auto expr = expression(fact, prec);
        TRY(expr)

        return unary({ start, m_endLoc }, kind, expr);
    }

    return makeError(Diag::expectedExpression, m_token.description());
}

Result<AstExpr*> Parser::unary(llvm::SMRange range, TokenKind op, AstExpr* expr) {
    switch (op) {
    case TokenKind::Dereference:
        return m_context.create<AstDereference>(range, expr);
    case TokenKind::AddressOf:
        return m_context.create<AstAddressOf>(range, expr);
    default:
        return m_context.create<AstUnaryExpr>(range, op, expr);
    }
}

Result<AstExpr*> Parser::binary(llvm::SMRange range, TokenKind op, AstExpr* lhs, AstExpr* rhs) {
    switch (op) {
    case TokenKind::CommaAnd:
        return m_context.create<AstBinaryExpr>(range, TokenKind::LogicalAnd, lhs, rhs);
    case TokenKind::Assign:
        return m_context.create<AstAssignExpr>(range, lhs, rhs);
    case TokenKind::MemberAccess:
        if (auto* member = llvm::dyn_cast<AstMemberAccess>(lhs)) {
            member->exprs.push_back(rhs);
            return member;
        } else {
            std::vector<AstExpr*> exprs{ lhs, rhs };
            return m_context.create<AstMemberAccess>(range, std::move(exprs));
        }
    default:
        return m_context.create<AstBinaryExpr>(range, op, lhs, rhs);
    }
}

/**
 * IdentExpr
 *   = id
 *   .
 */
Result<AstIdentExpr*> Parser::identifier() {
    auto start = m_token.range().Start;
    TRY(expect(TokenKind::Identifier))
    auto name = m_token.getStringValue();
    advance();

    return m_context.create<AstIdentExpr>(
        llvm::SMRange{ start, m_endLoc },
        name);
}

/**
 * callExpr = identifier "(" argList ")" .
 */
Result<AstCallExpr*> Parser::callExpr() {
    auto start = m_token.range().Start;
    auto id = identifier();
    TRY(id)

    TRY(consume(TokenKind::ParenOpen))
    auto args = expressionList();
    TRY(args)

    TRY(consume(TokenKind::ParenClose))

    return m_context.create<AstCallExpr>(
        llvm::SMRange{ start, m_endLoc },
        id,
        args);
}

/**
 * IfExpr = "IF" expr "THEN" expr "ELSE" expr .
 */
Result<AstIfExpr*> Parser::ifExpr() {
    // assume m_token == IF
    auto start = m_token.range().Start;
    advance();

    auto expr = expression(ExprFlags::commaAsAnd);
    TRY(expr)

    TRY(consume(TokenKind::Then))
    auto trueExpr = expression();
    TRY(trueExpr)

    TRY(consume(TokenKind::Else))
    auto falseExpr = expression();
    TRY(falseExpr)

    return m_context.create<AstIfExpr>(
        llvm::SMRange{ start, m_endLoc },
        expr,
        trueExpr,
        falseExpr);
}

/**
 * literal = stringLiteral
 *         | IntegerLiteral
 *         | FloatingPointLiteral
 *         | "TRUE"
 *         | "FALSE"
 *         | "NULL"
 *         .
 */
Result<AstLiteralExpr*> Parser::literal() {
    auto token = m_token;
    advance();

    return m_context.create<AstLiteralExpr>(
        token.range(),
        token.getValue());
}

/**
 * Parse comma separated list of expressionds
 */
Result<AstExprList*> Parser::expressionList() {
    auto start = m_token.range().Start;
    std::vector<AstExpr*> exprs;

    while (!m_token.isOneOf(TokenKind::EndOfFile, TokenKind::ParenClose, TokenKind::EndOfStmt)) {
        auto expr = expression();
        TRY(expr)

        exprs.emplace_back(expr);
        if (!accept(TokenKind::Comma)) {
            break;
        }
    }

    return m_context.create<AstExprList>(
        llvm::SMRange{ start, m_endLoc },
        std::move(exprs));
}

//----------------------------------------
// Helpers
//----------------------------------------

void Parser::replace(TokenKind what, TokenKind with) {
    if (m_token.is(what)) {
        m_token.setKind(with);
    }
}

[[nodiscard]] Result<void> Parser::expect(TokenKind kind) const {
    if (m_token.is(kind)) {
        return {};
    }

    return makeError(Diag::unexpectedToken, Token::description(kind), m_token.description());
}

void Parser::advance() {
    m_endLoc = m_token.range().End;
    m_lexer.next(m_token);
}
