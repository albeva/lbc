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

namespace {
inline auto getStart(const AstAttributeList* attribs, const Token& token) -> llvm::SMLoc {
    if (attribs == nullptr) {
        return token.getRange().Start;
    }
    return attribs->range.Start;
}
} // namespace

Parser::Parser(Context& context, Lexer& lexer, bool isMain, SymbolTable* symbolTable)
: ErrorLogger(context.getDiag()),
  m_context{ context },
  m_lexer{ lexer },
  m_isMain{ isMain },
  m_symbolTable{ symbolTable },
  m_language{ CallingConv::Default },
  m_scope{ Scope::Root } {
    advance();
}

void Parser::reset() {
    m_scope = Scope::Root;
    m_exprFlags = {};
    m_endLoc = {};
    m_token = {};
    m_controlStack.clear();
    advance();
    m_endLoc = m_token.getRange().Start;
}

/**
 * Module
 *   = StmtList
 *   .
 */
auto Parser::parse() -> Result<AstModule*> {
    TRY_DECL(stmts, stmtList())

    return m_context.create<AstModule>(
        m_lexer.getFileId(),
        stmts->range,
        m_isMain,
        std::move(m_imports),
        stmts
    );
}

//----------------------------------------
// Statements
//----------------------------------------

/**
 * StmtList
 *   = { Statement }
 *   .
 */
auto Parser::stmtList() -> Result<AstStmtList*> {
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

    auto start = m_token.getRange().Start;
    std::vector<AstDecl*> decls;
    std::vector<AstFuncStmt*> funcs;
    std::vector<AstStmt*> stms;

    while (isNonTerminator(m_token)) {
        TRY_DECL(stmt, statement())

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
        std::move(stms)
    );
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
auto Parser::statement() -> Result<AstStmt*> {
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
            return makeError(Diag::notAllowedTopLevelStatement, m_token);
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

    TRY_DECL(expr, expression(ExprFlags::useAssign | ExprFlags::callWithoutParens))
    return m_context.create<AstExprStmt>(expr->range, expr);
}

/**
 * IMPORT
 *   = "IMPORT" id
 *   .
 */
auto Parser::kwImport() -> Result<AstImport*> {
    // assume m_token == Import
    advance();

    TRY(expect(TokenKind::Identifier))

    auto import = m_token.lexeme();
    auto range = m_token.getRange();
    advance();

    // Imported file
    auto source = m_context.getOptions().getCompilerDir() / "lib" / (import + ".bas").str();
    if (!m_context.import(source.string())) {
        return m_context.create<AstImport>(llvm::SMRange{ range.Start, m_endLoc }, import);
    }
    if (!fs::exists(source)) {
        getDiag().log(Diag::moduleNotFound, range.Start, range, import);
        return ResultError{};
    }

    // Load import into Source Mgr
    std::string included;
    auto ID = m_context.getSourceMrg().AddIncludeFile(
        source.string(),
        range.Start,
        included
    );
    if (ID == ~0U) {
        getDiag().log(Diag::failedToLoadModule, range.Start, range, source.string());
        return ResultError{};
    }

    // parse the module
    Lexer lexer{ m_context, ID };
    TRY_DECL(module, Parser(m_context, lexer, false).parse())

    return m_context.create<AstImport>(
        llvm::SMRange{ range.Start, m_endLoc },
        import,
        module
    );
}

/**
 * Extern
 *   = [ LanguageStringLiteral ]
 *   ( Statement
 *   | StatementList "END" "EXTERN"
 *   .
 */
auto Parser::kwExtern() -> Result<AstExtern*> {
    // assume m_token == Extern
    auto start = m_token.getRange().Start;
    advance();
    RESTORE_ON_EXIT(m_language);

    if (m_token.is(TokenKind::StringLiteral)) {
        auto str = m_token.getStringValue().upper();
        if (str == "C") {
            m_language = CallingConv::C;
        } else if (str == "DEFAULT") {
            m_language = CallingConv::Default;
        } else {
            return makeError(Diag::unsupportedExternLanguage, m_token, str);
        }
        advance();
    }

    std::vector<AstStmt*> stmts{};

    if (accept(TokenKind::EndOfStmt)) {
        while (m_token.isNot(TokenKind::End)) {
            TRY_DECL(decl, declaration())
            if (decl == nullptr) {
                return makeError(Diag::onlyDeclarationsInExtern, m_token);
            }
            stmts.emplace_back(decl);
            TRY(consume(TokenKind::EndOfStmt))
        }
        TRY(consume(TokenKind::End))
        TRY(consume(TokenKind::Extern))
    } else {
        TRY_DECL(decl, declaration())
        stmts.emplace_back(decl);
    }

    return m_context.create<AstExtern>(
        llvm::SMRange{ start, m_endLoc },
        m_language,
        std::move(stmts)
    );
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
auto Parser::declaration() -> Result<AstStmt*> {
    TRY_DECL(attribs, attributeList())

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
        return makeError(Diag::expectedDeclarationAfterAttribute, m_token, m_token.description());
    }
    return nullptr;
}

//----------------------------------------
// Attributes
//----------------------------------------

/**
 *  AttributeList = [ '[' Attribute { ','  Attribute } ']' ].
 */
auto Parser::attributeList() -> Result<AstAttributeList*> {
    if (m_token.isNot(TokenKind::BracketOpen)) {
        return nullptr;
    }

    auto start = m_token.getRange().Start;
    advance();

    std::vector<AstAttribute*> attribs;

    while(true) {
        TRY_DECL(attrib, attribute())
        attribs.emplace_back(attrib);
        if (!acceptComma()) {
            break;
        }
    }

    TRY(consume(TokenKind::BracketClose))

    return m_context.create<AstAttributeList>(
        llvm::SMRange{ start, m_endLoc },
        std::move(attribs)
    );
}

/**
 * Attribute
 *   = IdentExpr [ AttributeArgList ]
 *   .
 */
auto Parser::attribute() -> Result<AstAttribute*> {
    auto start = m_token.getRange().Start;

    TRY_DECL(id, identifier())

    AstExprList* args = nullptr;
    if (m_token.isOneOf(TokenKind::AssignOrEqual, TokenKind::ParenOpen)) {
        TRY_ASSIGN(args, attributeArgList())
    }

    return m_context.create<AstAttribute>(
        llvm::SMRange{ start, m_endLoc },
        id,
        args
    );
}

/**
 * AttributeArgList
 *   = "=" Literal
 *   | "(" [ Literal { "," Literal } ] ")"
 *   .
 */
auto Parser::attributeArgList() -> Result<AstExprList*> {
    auto start = m_token.getRange().Start;
    std::vector<AstExpr*> args;

    if (acceptAssign()) {
        TRY_DECL(lit, literal())
        args.emplace_back(lit);
    } else if (accept(TokenKind::ParenOpen)) {
        while (!m_token.isOneOf(TokenKind::EndOfFile, TokenKind::ParenClose)) {
            TRY_DECL(lit, literal())
            args.emplace_back(lit);
            if (!acceptComma()) {
                break;
            }
        }
        TRY(consume(TokenKind::ParenClose))
    }

    return m_context.create<AstExprList>(
        llvm::SMRange{ start, m_endLoc },
        std::move(args)
    );
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
auto Parser::kwDim(AstAttributeList* attribs) -> Result<AstVarDecl*> {
    // assume m_token == VAR
    auto start = getStart(attribs, m_token);
    advance();

    TRY(expect(TokenKind::Identifier))
    auto id = m_token.getStringValue();
    auto token = m_token;
    advance();

    AstTypeExpr* type = nullptr;
    AstExpr* expr = nullptr;

    if (accept(TokenKind::As)) {
        TRY_ASSIGN(type, typeExpr())

        if (acceptAssign()) {
            TRY_ASSIGN(expr, expression())
        }
    } else {
        TRY(consumeAssign())
        TRY_ASSIGN(expr, expression())
    }

    return m_context.create<AstVarDecl>(
        llvm::SMRange{ start, m_endLoc },
        id,
        token,
        m_language,
        attribs,
        type,
        expr
    );
}

//----------------------------------------
// DECLARE
//----------------------------------------

/**
 * DECLARE
 *   = "DECLARE" FuncSignature
 *   .
 */
auto Parser::kwDeclare(AstAttributeList* attribs) -> Result<AstFuncDecl*> {
    // assume m_token == DECLARE
    if (m_scope != Scope::Root) {
        return makeError(Diag::unexpectedNestedDeclaration, m_token, m_token.description());
    }
    auto start = getStart(attribs, m_token);
    advance();

    return funcSignature(start, attribs, FuncFlags::isDeclaration);
}

/**
 * FuncSignature
 *     = "FUNCTION" id [ "(" [ FuncParamList ] ")" ] "AS" TypeExpr
 *     | "SUB" id [ "(" FuncParamList ")" ]
 *     .
 */
auto Parser::funcSignature(llvm::SMLoc start, AstAttributeList* attribs, FuncFlags funcFlags) -> Result<AstFuncDecl*> {
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
    AstFuncParamList* params = nullptr;
    if (accept(TokenKind::ParenOpen)) {
        TRY_ASSIGN(params, funcParamList(isVariadic, flags::has(funcFlags, FuncFlags::isAnonymous)))
        TRY(consume(TokenKind::ParenClose))
    }

    AstTypeExpr* ret = nullptr;
    if (isFunc) {
        TRY(consume(TokenKind::As))
        TRY_ASSIGN(ret, typeExpr())
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
        !flags::has(funcFlags, FuncFlags::isDeclaration)
    );
}

/**
 * FuncParamList
 *   = FuncParam { "," FuncParam } [ "," "..." ]
 *   | "..."
 *   .
 */
auto Parser::funcParamList(bool& isVariadic, bool isAnonymous) -> Result<AstFuncParamList*> {
    auto start = m_token.getRange().Start;
    std::vector<AstFuncParamDecl*> params;
    while (!m_token.isOneOf(TokenKind::EndOfFile, TokenKind::ParenClose)) {
        if (accept(TokenKind::Ellipsis)) {
            isVariadic = true;
            if (m_token.is(TokenKind::CommaOrConditionAnd)) {
                return makeError(Diag::variadicArgumentNotLast, m_token);
            }
            break;
        }
        TRY_DECL(param, funcParam(isAnonymous))

        params.push_back(param);
        if (!acceptComma()) {
            break;
        }
    }

    return m_context.create<AstFuncParamList>(
        llvm::SMRange{ start, m_endLoc },
        std::move(params)
    );
}

/**
 * FuncParam
 *  = id "AS" TypeExpr
 *  | TypeExpr        // if isAnonymous
 *  .
 */
auto Parser::funcParam(bool isAnonymous) -> Result<AstFuncParamDecl*> {
    auto start = m_token.getRange().Start;

    llvm::StringRef id;
    Token token;
    if (isAnonymous) {
        if (m_token.is(TokenKind::Identifier)) {
            token = m_token;
            id = token.getStringValue();
            TRY(consume(TokenKind::As))
        } else {
            id = "";
        }
    } else {
        TRY(expect(TokenKind::Identifier))
        token = m_token;
        id = token.getStringValue();
        advance();
        TRY(consume(TokenKind::As))
    }

    TRY_DECL(type, typeExpr())

    return m_context.create<AstFuncParamDecl>(
        llvm::SMRange{ start, m_endLoc },
        id,
        token,
        m_language,
        nullptr,
        type
    );
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
auto Parser::kwType(AstAttributeList* attribs) -> Result<AstDecl*> {
    // assume m_token == TYPE
    auto start = m_token.getRange().Start;
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

    return makeError(Diag::unexpectedToken, m_token, "'=' or end of statement", m_token.description());
}

/**
 *  alias
 *    = TypeExpr
 *    .
 */
auto Parser::alias(llvm::StringRef id, Token token, llvm::SMLoc start, AstAttributeList* attribs) -> Result<AstTypeAlias*> {
    TRY_DECL(type, typeExpr())

    return m_context.create<AstTypeAlias>(
        llvm::SMRange{ start, m_endLoc },
        id,
        token,
        m_language,
        attribs,
        type
    );
}

/**
 * UDT
 *   = EoS
 *     udtDeclList
 *     "END" "TYPE"
 *   .
 */
auto Parser::udt(llvm::StringRef id, Token token, llvm::SMLoc start, AstAttributeList* attribs) -> Result<AstUdtDecl*> {
    // assume m_token == declaration || "end"
    TRY_DECL(decls, udtDeclList())

    TRY(consume(TokenKind::End))
    TRY(consume(TokenKind::Type))

    return m_context.create<AstUdtDecl>(
        llvm::SMRange{ start, m_endLoc },
        id,
        token,
        m_language,
        attribs,
        decls
    );
}

/**
 * udtDeclList
 *   = { [ AttributeList ] udtMember EoS }
 *   .
 */
auto Parser::udtDeclList() -> Result<AstDeclList*> {
    auto start = m_token.getRange().Start;
    std::vector<AstDecl*> decls;

    while (true) {
        TRY_DECL(attribs, attributeList())

        if (attribs != nullptr) {
            TRY(expect(TokenKind::Identifier))
        } else if (m_token.isNot(TokenKind::Identifier)) {
            break;
        }

        TRY_DECL(member, udtMember(attribs))

        decls.emplace_back(member);
        TRY(consume(TokenKind::EndOfStmt))
    }

    return m_context.create<AstDeclList>(
        llvm::SMRange{ start, m_endLoc },
        std::move(decls)
    );
}

/**
 * udtMember
 *   = id "AS" TypeExpr
 *   .
 */
auto Parser::udtMember(AstAttributeList* attribs) -> Result<AstDecl*> {
    // assume m_token == Identifier
    auto start = m_token.getRange().Start;
    auto id = m_token.getStringValue();
    auto token = m_token;
    advance();

    TRY(consume(TokenKind::As))
    TRY_DECL(type, typeExpr())

    return m_context.create<AstVarDecl>(
        llvm::SMRange{ start, m_endLoc },
        id,
        token,
        m_language,
        attribs,
        type,
        nullptr
    );
}

//----------------------------------------
// Call
//----------------------------------------

/**
 *  FUNCTION = funcSignature <EoS>
 *             stmtList
 *             "END" ("FUNCTION" | "SUB")
 */
auto Parser::kwFunction(AstAttributeList* attribs) -> Result<AstFuncStmt*> {
    if (m_scope != Scope::Root) {
        return makeError(Diag::unexpectedNestedDeclaration, m_token, m_token.description());
    }

    bool const isFunction = m_token.is(TokenKind::Function);
    auto start = getStart(attribs, m_token);
    TRY_DECL(decl, funcSignature(start, attribs, {}))

    RESTORE_ON_EXIT(m_scope);
    m_scope = Scope::Function;
    AstStmtList* stmts = nullptr;

    if (accept(TokenKind::LambdaBody)) {
        AstStmt* stmt = nullptr;
        if (isFunction) {
            TRY_DECL(expr, expression())
            stmt = m_context.create<AstReturnStmt>(
                llvm::SMRange{ start, m_endLoc },
                expr
            );
        } else {
            TRY_ASSIGN(stmt, statement())
        }
        stmts = m_context.create<AstStmtList>(
            llvm::SMRange{ start, m_endLoc },
            std::vector<AstDecl*>{}, // TODO: Fix. stmt could be a declaration!
            std::vector<AstFuncStmt*>{},
            std::vector<AstStmt*>{ stmt }
        );
    } else {
        TRY(consume(TokenKind::EndOfStmt))
        TRY_ASSIGN(stmts, stmtList())

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
        stmts
    );
}

/**
 * RETURN = "RETURN" [ expression ] .
 */
auto Parser::kwReturn() -> Result<AstStmt*> {
    // assume m_token == RETURN
    if (m_scope == Scope::Root && !m_isMain) {
        return makeError(Diag::unexpectedReturn, m_token);
    }
    auto start = m_token.getRange().Start;
    advance();

    AstExpr* expr = nullptr;
    if (m_token.isNot(TokenKind::EndOfStmt)) {
        TRY_ASSIGN(expr, expression())
    }

    return m_context.create<AstReturnStmt>(
        llvm::SMRange{ start, m_endLoc },
        expr
    );
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
auto Parser::kwIf() -> Result<AstIfStmt*> {
    // assume m_token == IF
    auto start = m_token.getRange().Start;
    advance();

    std::vector<AstIfStmtBlock*> blocks;
    TRY_DECL(block, ifBlock())

    blocks.emplace_back(block);

    if (m_token.is(TokenKind::EndOfStmt)) {
        acceptNext(TokenKind::Else);
    }

    while (accept(TokenKind::Else)) {
        if (accept(TokenKind::If)) {
            TRY_DECL(if_, ifBlock())
            blocks.emplace_back(if_);
        } else {
            TRY_DECL(else_, thenBlock({}, nullptr))
            blocks.emplace_back(else_);
        }

        if (m_token.is(TokenKind::EndOfStmt)) {
            acceptNext(TokenKind::Else);
        }
    }

    if (blocks.back()->stmt->kind == AstKind::StmtList) {
        TRY(consume(TokenKind::End))
        TRY(consume(TokenKind::If))
    }

    return m_context.create<AstIfStmt>(
        llvm::SMRange{ start, m_endLoc },
        std::move(blocks)
    );
}

/**
 * IfBlock
 *   = [ VAR { "," VAR } "," ] Expression "THEN" ThenBlock
 *   .
 */
auto Parser::ifBlock() -> Result<AstIfStmtBlock*> {
    std::vector<AstVarDecl*> decls;
    while (m_token.is(TokenKind::Dim)) {
        TRY_DECL(var, kwDim(nullptr))

        decls.emplace_back(var);
        TRY(consumeComma())
    }
    TRY_DECL(expr, expression(ExprFlags::commaAsAnd))
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
auto Parser::thenBlock(std::vector<AstVarDecl*> decls, AstExpr* expr) -> Result<AstIfStmtBlock*> {
    AstStmt* stmt = nullptr;
    if (accept(TokenKind::EndOfStmt)) {
        TRY_ASSIGN(stmt, stmtList())
    } else {
        TRY_ASSIGN(stmt, statement())
    }

    return m_context.create<AstIfStmtBlock>(
        std::move(decls),
        nullptr,
        expr,
        stmt
    );
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
auto Parser::kwFor() -> Result<AstForStmt*> {
    // assume m_token == FOR
    auto start = m_token.getRange().Start;
    advance();

    std::vector<AstVarDecl*> decls;

    // [ VAR { "," VAR } "," ]
    while (m_token.is(TokenKind::Dim)) {
        TRY_DECL(var, kwDim(nullptr))

        decls.emplace_back(var);
        TRY(consumeComma())
    }

    // id [ "AS" TypeExpr ] "=" Expression
    auto idStart = m_token.getRange().Start;
    TRY(expect(TokenKind::Identifier))
    auto id = m_token.getStringValue();
    auto token = m_token;
    advance();

    AstTypeExpr* type = nullptr;
    if (accept(TokenKind::As)) {
        TRY_ASSIGN(type, typeExpr())
    }

    TRY(consumeAssign())
    TRY_DECL(expr, expression())

    auto* iterator = m_context.create<AstVarDecl>(
        llvm::SMRange{ idStart, m_endLoc },
        id,
        token,
        m_language,
        nullptr,
        type,
        expr
    );

    // "TO" Expression [ "STEP" expression ]
    TRY(consume(TokenKind::To))
    TRY_DECL(limit, expression())

    AstExpr* step = nullptr;
    if (accept(TokenKind::Step)) {
        TRY_ASSIGN(step, expression())
    }

    // "=>" statement ?
    AstStmt* stmt = nullptr;
    llvm::StringRef next;
    if (accept(TokenKind::LambdaBody)) {
        TRY_ASSIGN(stmt, m_controlStack.with(ControlFlowStatement::For, [&] {
            return statement();
        }))
    } else {
        TRY(consume(TokenKind::EndOfStmt))
        TRY_ASSIGN(stmt, m_controlStack.with(ControlFlowStatement::For, [&] {
            return stmtList();
        }))

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
        next
    );
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
auto Parser::kwDo() -> Result<AstDoLoopStmt*> {
    // assume m_token == DO
    auto start = m_token.getRange().Start;
    advance();

    auto condition = AstDoLoopStmt::Condition::None;
    AstStmt* stmt = nullptr;
    AstExpr* expr = nullptr;
    std::vector<AstVarDecl*> decls;

    // [ VAR { "," VAR } ]
    auto canHaveComma = false;
    while (m_token.is(TokenKind::Dim) || (canHaveComma && acceptComma())) {
        canHaveComma = true;
        TRY_DECL(var, kwDim(nullptr))
        decls.emplace_back(var);
    }

    // ( EoS StmtList "LOOP" [ Condition ]
    if (accept(TokenKind::EndOfStmt)) {
        TRY_ASSIGN(stmt, m_controlStack.with(ControlFlowStatement::Do, [&] {
            return stmtList();
        }))

        TRY(consume(TokenKind::Loop))

        // [ Condition ]
        if (accept(TokenKind::Until)) {
            condition = AstDoLoopStmt::Condition::PostUntil;
            TRY_ASSIGN(expr, expression(ExprFlags::commaAsAnd))
        } else if (accept(TokenKind::While)) {
            condition = AstDoLoopStmt::Condition::PostWhile;
            TRY_ASSIGN(expr, expression(ExprFlags::commaAsAnd))
        }
    } else {
        // [ Condition ]
        if (accept(TokenKind::Until)) {
            condition = AstDoLoopStmt::Condition::PreUntil;
            TRY_ASSIGN(expr, expression(ExprFlags::commaAsAnd))
        } else if (accept(TokenKind::While)) {
            condition = AstDoLoopStmt::Condition::PreWhile;
            TRY_ASSIGN(expr, expression(ExprFlags::commaAsAnd))
        }

        // EoS StmtList "LOOP"
        if (accept(TokenKind::EndOfStmt)) {
            TRY_ASSIGN(stmt, m_controlStack.with(ControlFlowStatement::Do, [&] {
                return stmtList();
            }))
            TRY(consume(TokenKind::Loop))
        }
        // "=>" Statement
        else {
            TRY(consume(TokenKind::LambdaBody))
            TRY_ASSIGN(stmt, m_controlStack.with(ControlFlowStatement::Do, [&] {
                return statement();
            }))
        }
    }

    return m_context.create<AstDoLoopStmt>(
        llvm::SMRange{ start, m_endLoc },
        std::move(decls),
        condition,
        expr,
        stmt
    );
}

//----------------------------------------
// Branching
//----------------------------------------

/**
 * CONTINUE
 *   = "CONTINUE" { "FOR" | "DO" }
 *   .
 */
auto Parser::kwContinue() -> Result<AstContinuationStmt*> {
    return continuation(AstContinuationAction::Continue);
}

/**
 * EXIT
 *   = "EXIT" { "FOR" }
 *   .
 */
auto Parser::kwExit() -> Result<AstContinuationStmt*> {
    return continuation(AstContinuationAction::Exit);
}

auto Parser::continuation(AstContinuationAction action) -> Result<AstContinuationStmt*> {
    // assume m_token == CONTINUE | EXIT
    auto start = m_token.getRange().Start;
    auto control = m_token.description();

    if (m_controlStack.empty()) {
        return makeError(Diag::unexpectedContinuation, m_token, control);
    }

    advance();

    auto iter = m_controlStack.begin();
    auto index = m_controlStack.indexOf(iter);
    const auto target = [&](ControlFlowStatement look) -> Result<void> {
        iter = m_controlStack.find(iter, look);
        if (iter == m_controlStack.end()) {
            return makeError(Diag::unexpectedContinuationTarget, m_token, control, m_token.description());
        }
        index = m_controlStack.indexOf(iter);
        iter++;
        advance();
        return {};
    };

    while (true) {
        switch (m_token.getKind()) {
        case TokenKind::For:
            TRY(target(ControlFlowStatement::For))
            continue;
        case TokenKind::Do:
            TRY(target(ControlFlowStatement::Do))
            continue;
        default:
            break;
        }
        break;
    }

    return m_context.create<AstContinuationStmt>(
        llvm::SMRange{ start, m_endLoc },
        action,
        index
    );
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
auto Parser::typeExpr() -> Result<AstTypeExpr*> {
    auto start = m_token.getRange().Start;
    bool const parenthesized = accept(TokenKind::ParenOpen);
    bool mustBePtr = false;

    AstTypeExpr::TypeExpr expr;
    if (m_token.isOneOf(TokenKind::Sub, TokenKind::Function)) {
        TRY_ASSIGN(expr, funcSignature(start, nullptr, FuncFlags::isAnonymous))
        mustBePtr = true;
    } else if (m_token.is(TokenKind::Any) || m_token.isTypeKeyword()) {
        expr = m_token.getKind();
        advance();
    } else if (m_token.is(TokenKind::TypeOf)) {
        TRY_ASSIGN(expr, kwTypeOf())
    } else {
        TRY_DECL(ident, identifier())

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
        deref
    );
}

/**
 * TypeOf = "TYPEOF" "(" (Expr | TypeExpr) ")"
 *        .
 */
auto Parser::kwTypeOf() -> Result<AstTypeOf*> {
    // assume m_token == "TYPEOF"
    auto start = m_token.getRange().Start;
    advance();

    TRY(consume(TokenKind::ParenOpen))
    llvm::SMLoc exprLoc = m_token.getRange().Start;
    int parens = 1;
    while (true) {
        if (m_token.isOneOf(TokenKind::EndOfStmt, TokenKind::EndOfFile, TokenKind::Invalid)) {
            return makeError(Diag::unexpectedToken, m_token, "type expression", m_token.description());
        }
        if (m_token.is(TokenKind::ParenClose)) {
            parens--;
            if (parens == 0) {
                advance();
                break;
            }
            if (parens < 0) {
                return makeError(Diag::unexpectedToken, m_token, "type expression", m_token.description());
            }
        } else if (m_token.is(TokenKind::ParenOpen)) {
            parens++;
        }
        advance();
    }
    return m_context.create<AstTypeOf>(llvm::SMRange{ start, m_endLoc }, exprLoc);
}

//----------------------------------------
// Expressions
//----------------------------------------

/**
 * expression = factor { <Binary Op> expression }
 *            . [ ArgumentList ]
 */
auto Parser::expression(ExprFlags flags) -> Result<AstExpr*> {
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
    TRY_DECL(expr, factor())

    resolveBinaryOperators();

    // expr op
    if (m_token.isOperator()) {
        TRY_ASSIGN(expr, expression(expr, 1))
    }

    // print "hello"
    if (flags::has(flags, ExprFlags::callWithoutParens) && llvm::isa<AstIdentExpr>(*expr) && allowCallWithToken(m_token)) {
        auto start = expr->range.Start;
        TRY_DECL(args, expressionList())

        expr = m_context.create<AstCallExpr>(
            llvm::SMRange{ start, m_endLoc },
            expr,
            args
        );
    }

    return expr;
}

/**
 * Recursievly climb operator precedence
 * https://en.wikipedia.org/wiki/Operator-precedence_parser#Precedence_climbing_method
 */
auto Parser::expression(AstExpr* lhs, int precedence) -> Result<AstExpr*> {
    while (m_token.getPrecedence() >= precedence) {
        if (!m_token.isBinary()) {
            return makeError(Diag::unexpectedToken, m_token, "binary operator", m_token.description());
        }
        auto current = m_token.getPrecedence();
        auto op = m_token;
        advance();

        TRY_DECL(rhs, factor())

        resolveBinaryOperators();

        while (m_token.getPrecedence() > current || (m_token.isRightToLeft() && m_token.getPrecedence() == current)) {
            TRY_ASSIGN(rhs, expression(rhs, m_token.getPrecedence()))
        }

        auto start = lhs->range.Start;
        TRY_ASSIGN(lhs, binary({ start, m_endLoc }, op, lhs, rhs))
    }
    return lhs;
}

void Parser::resolveBinaryOperators() {
    switch (m_token.getKind()) {
    case TokenKind::AssignOrEqual:
        if (flags::has(m_exprFlags, ExprFlags::useAssign)) {
            flags::unset(m_exprFlags, ExprFlags::useAssign);
            m_token.setKind(TokenKind::Assign);
        } else {
            m_token.setKind(TokenKind::Equal);
        }
        break;
    case TokenKind::CommaOrConditionAnd:
        if (flags::has(m_exprFlags, ExprFlags::commaAsAnd)) {
            m_token.setKind(TokenKind::ConditionAnd);
        } else {
            m_token.setKind(TokenKind::Comma);
        }
        break;
    case TokenKind::MinusOrNegate:
        m_token.setKind(TokenKind::Minus);
        break;
    case TokenKind::MultiplyOrDereference:
        m_token.setKind(TokenKind::Multiply);
        break;
    default:
        break;
    }
}

/**
 * factor = primary { "(" expressionList ")" | "AS" TypeExpr | } .
 */
auto Parser::factor() -> Result<AstExpr*> {
    auto start = m_token.getRange().Start;
    TRY_DECL(expr, primary())

    while (true) {
        // Note: Currently have no Right-To-Left unary operators, otherwise handle them here.

        if (accept(TokenKind::ParenOpen)) {
            TRY_DECL(args, expressionList())

            TRY(consume(TokenKind::ParenClose))
            expr = m_context.create<AstCallExpr>(
                llvm::SMRange{ start, m_endLoc },
                expr,
                args
            );
            continue;
        }

        // "AS" TypeExpr
        if (accept(TokenKind::As)) {
            TRY_DECL(type, typeExpr())

            auto* cast = m_context.create<AstCastExpr>(
                llvm::SMRange{ start, m_endLoc },
                expr,
                type,
                false
            );
            expr = cast;
            continue;
        }
        break;
    }
    return expr;
}

/**
 * primary = literal
 *         | identifier
 *         | "(" expression ")"
 *         | IfExpr
 *         | <Left Unary Op> [ factor { <Binary Op> expression } ]
 *         | IfExpr
 *        .
 */
auto Parser::primary() -> Result<AstExpr*> {
    if (m_token.isLiteral()) {
        return literal();
    }

    switch (m_token.getKind()) {
    case TokenKind::Identifier:
        return identifier();
    case TokenKind::ParenOpen: {
        advance();
        TRY_DECL(expr, expression())
        TRY(consume(TokenKind::ParenClose))
        return expr;
    }
    case TokenKind::If:
        return ifExpr();
    case TokenKind::MultiplyOrDereference:
        m_token.setKind(TokenKind::Dereference);
        break;
    case TokenKind::MinusOrNegate:
        m_token.setKind(TokenKind::Negate);
        break;
    default:
        break;
    }

    if (m_token.isUnary() && m_token.isLeftToRight()) {
        auto tkn = m_token;
        advance();

        TRY_DECL(fact, factor())
        TRY_DECL(expr, expression(fact, tkn.getPrecedence()))

        return unary({ tkn.getRange().Start, m_endLoc }, tkn, expr);
    }

    return makeError(Diag::expectedExpression, m_token, m_token.description());
}

auto Parser::unary(llvm::SMRange range, const Token& tkn, AstExpr* expr) -> Result<AstExpr*> {
    switch (tkn.getKind()) {
    case TokenKind::Dereference:
        return m_context.create<AstDereference>(range, expr);
    case TokenKind::AddressOf:
        return m_context.create<AstAddressOf>(range, expr);
    default:
        return m_context.create<AstUnaryExpr>(range, tkn, expr);
    }
}

auto Parser::binary(llvm::SMRange range, const Token& tkn, AstExpr* lhs, AstExpr* rhs) -> Result<AstExpr*> {
    switch (tkn.getKind()) {
    case TokenKind::ConditionAnd: {
        auto copy = tkn;
        copy.setKind(TokenKind::LogicalAnd);
        return m_context.create<AstBinaryExpr>(range, copy, lhs, rhs);
    }
    case TokenKind::MemberAccess:
        return m_context.create<AstMemberExpr>(range, tkn, lhs, rhs);
    case TokenKind::Assign:
        return m_context.create<AstAssignExpr>(range, lhs, rhs);
    default:
        return m_context.create<AstBinaryExpr>(range, tkn, lhs, rhs);
    }
}

/**
 * IdentExpr
 *   = id
 *   .
 */
auto Parser::identifier() -> Result<AstIdentExpr*> {
    auto start = m_token.getRange().Start;
    TRY(expect(TokenKind::Identifier))
    auto name = m_token.getStringValue();
    advance();

    return m_context.create<AstIdentExpr>(
        llvm::SMRange{ start, m_endLoc },
        name
    );
}

/**
 * IfExpr = "IF" expr "THEN" expr "ELSE" expr .
 */
auto Parser::ifExpr() -> Result<AstIfExpr*> {
    // assume m_token == IF
    auto start = m_token.getRange().Start;
    advance();

    TRY_DECL(expr, expression(ExprFlags::commaAsAnd))

    TRY(consume(TokenKind::Then))
    TRY_DECL(trueExpr, expression())

    TRY(consume(TokenKind::Else))
    TRY_DECL(falseExpr, expression())

    return m_context.create<AstIfExpr>(
        llvm::SMRange{ start, m_endLoc },
        expr,
        trueExpr,
        falseExpr
    );
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
auto Parser::literal() -> Result<AstLiteralExpr*> {
    auto token = m_token;
    advance();

    return m_context.create<AstLiteralExpr>(
        token.getRange(),
        token.getValue()
    );
}

/**
 * Parse comma separated list of expressionds
 */
auto Parser::expressionList() -> Result<AstExprList*> {
    auto start = m_token.getRange().Start;
    std::vector<AstExpr*> exprs;

    while (!m_token.isOneOf(TokenKind::EndOfFile, TokenKind::ParenClose, TokenKind::EndOfStmt)) {
        TRY_DECL(expr, expression())

        exprs.emplace_back(expr);
        if (!acceptComma()) {
            break;
        }
    }

    return m_context.create<AstExprList>(
        llvm::SMRange{ start, m_endLoc },
        std::move(exprs)
    );
}

//----------------------------------------
// Helpers
//----------------------------------------

auto Parser::accept(TokenKind kind) -> bool {
    if (m_token.is(kind)) {
        advance();
        return true;
    }
    return false;
}

auto Parser::acceptAssign() -> bool {
    if (m_token.isOneOf(TokenKind::AssignOrEqual, TokenKind::Assign)) {
        advance();
        return true;
    }
    return false;
}

auto Parser::acceptComma() -> bool {
    if (m_token.isOneOf(TokenKind::CommaOrConditionAnd, TokenKind::Comma)) {
        advance();
        return true;
    }
    return false;
}

auto Parser::acceptNext(TokenKind kind) -> bool {
    Lexer peek{ m_lexer };

    if (const Token token = peek.next(); token.is(kind)) {
        m_token = token;
        m_lexer = peek;
        return true;
    }

    return false;
}

auto Parser::expect(TokenKind kind) const -> Result<void> {
    if (m_token.is(kind)) {
        return {};
    }

    return makeError(Diag::unexpectedToken, m_token, Token::description(kind), m_token.description());
}

auto Parser::consume(TokenKind kind) -> Result<void> {
    TRY(expect(kind))
    advance();
    return {};
}

auto Parser::consumeAssign() -> Result<void> {
    if (!acceptAssign()) {
        return makeError(Diag::unexpectedToken, m_token, Token::description(TokenKind::Assign), m_token.description());
    }
    return {};
}

auto Parser::consumeComma() -> Result<void> {
    if (!acceptComma()) {
        return makeError(Diag::unexpectedToken, m_token, Token::description(TokenKind::Comma), m_token.description());
    }
    return {};
}

void Parser::advance() {
    m_endLoc = m_token.getRange().End;
    m_token = m_lexer.next();
}
