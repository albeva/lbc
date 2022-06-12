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
#include "Lexer/TokenSource.hpp"
#include "Symbol/SymbolTable.hpp"
#include "Type/Type.hpp"
#include <llvm/ADT/TypeSwitch.h>
using namespace lbc;

Parser::Parser(Context& context, TokenSource& source, bool isMain, SymbolTable* symbolTable)
: m_context{ context },
  m_source{ source },
  m_isMain{ isMain },
  m_symbolTable{ symbolTable },
  m_diag{ context.getDiag() },
  m_scope{ Scope::Root } {
    advance();
}

void Parser::reset() noexcept {
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
    TRY_DECLARE(stmts, stmtList())
    return m_context.create<AstModule>(
        m_source.getFileId(),
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
    std::vector<AstDecl*> decl;
    std::vector<AstFuncStmt*> funcs;
    std::vector<AstStmt*> stms;

    while (isNonTerminator(m_token)) {
        TRY_DECLARE(stmt, statement())
        llvm::TypeSwitch<AstStmt*>(stmt)
            .Case<AstFuncStmt>([&](auto* node) {
                funcs.emplace_back(node);
                decl.emplace_back(node->decl);
            })
            .Case<AstDecl>([&](auto* node) {
                stms.emplace_back(node);
                decl.emplace_back(node);
            })
            .Case<AstImport>([&](auto* import) {
                m_imports.emplace_back(import);
            })
            .Default([&](auto* node) {
                stms.emplace_back(node);
            });
        TRY(consume(TokenKind::EndOfStmt))
    }

    return m_context.create<AstStmtList>(
        llvm::SMRange{ start, m_endLoc },
        std::move(decl),
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
    TRY_DECLARE(decl, declaration())
    if (decl != nullptr) {
        return decl;
    }

    if (m_scope == Scope::Root) {
        if (m_token.is(TokenKind::Import)) {
            return kwImport();
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

    TRY_DECLARE(expr, expression({ .useAssign = true, .callWithoutParens = true }))
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
        return makeError(range, Diag::moduleNotFound, import);
    }

    // Load import into Source Mgr
    std::string included;
    auto ID = m_context.getSourceMrg().AddIncludeFile(
        source.string(),
        range.Start,
        included);
    if (ID == ~0U) {
        return makeError(range, Diag::failedToLoadModule, source.string());
    }

    // parse the module
    Lexer lexer{ m_context, ID };
    TRY_DECLARE(module, Parser(m_context, lexer, false).parse())

    return m_context.create<AstImport>(
        llvm::SMRange{ range.Start, m_endLoc },
        import,
        module);
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
    TRY_DECLARE(attribs, attributeList())

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
        TRY_DECLARE(attrib, attribute())
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

    TRY_DECLARE(id, identifier())
    AstExprList* args = nullptr;
    if (m_token.isOneOf(TokenKind::Assign, TokenKind::ParenOpen)) {
        TRY_ASSIGN(args, attributeArgList())
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
        TRY_DECLARE(lit, literal())
        args.emplace_back(lit);
    } else if (accept(TokenKind::ParenOpen)) {
        while (!m_token.isOneOf(TokenKind::EndOfFile, TokenKind::ParenClose)) {
            TRY_DECLARE(lit, literal())
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
    advance();

    AstTypeExpr* type = nullptr;
    AstExpr* expr = nullptr;

    if (accept(TokenKind::As)) {
        TRY_ASSIGN(type, typeExpr({ .typeOfAllowsExpr = true }))
        if (accept(TokenKind::Assign)) {
            TRY_ASSIGN(expr, expression())
        }
    } else {
        TRY(consume(TokenKind::Assign))
        TRY_ASSIGN(expr, expression())
    }

    return m_context.create<AstVarDecl>(
        llvm::SMRange{ start, m_endLoc },
        id,
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

    return funcSignature(start, attribs, { .isDeclaration = true });
}

/**
 * FuncSignature
 *     = "FUNCTION" id [ "(" [ FuncParamList ] ")" ] "AS" TypeExpr
 *     | "SUB" id [ "(" FuncParamList ")" ]
 *     .
 */
Result<AstFuncDecl*> Parser::funcSignature(llvm::SMLoc start, AstAttributeList* attribs, FuncFlags funcFlags) {
    bool isFunc = accept(TokenKind::Function);
    if (!isFunc) {
        TRY(consume(TokenKind::Sub))
    }

    llvm::StringRef id;
    if (funcFlags.isAnonymous) {
        id = "";
    } else {
        TRY(expect(TokenKind::Identifier))
        id = m_token.getStringValue();
        advance();
    }

    bool isVariadic = false;
    AstFuncParamList* params = nullptr;
    if (accept(TokenKind::ParenOpen)) {
        TRY_ASSIGN(params, funcParamList(isVariadic, funcFlags.isAnonymous))
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
        attribs,
        params,
        isVariadic,
        ret,
        !funcFlags.isDeclaration);
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
        TRY_DECLARE(param, funcParam(isAnonymous))
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
    if (isAnonymous) {
        if (m_token.is(TokenKind::Identifier)) {
            Token next;
            m_source.peek(next);
            if (next.is(TokenKind::As)) {
                id = m_token.getStringValue();
                advance();
                advance();
            }
        } else {
            id = "";
        }
    } else {
        TRY(expect(TokenKind::Identifier))
        id = m_token.getStringValue();
        advance();
        TRY(consume(TokenKind::As))
    }

    TRY_DECLARE(type, typeExpr({ .typeOfAllowsExpr = false }))

    return m_context.create<AstFuncParamDecl>(
        llvm::SMRange{ start, m_endLoc },
        id,
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
    advance();

    if (accept(TokenKind::EndOfStmt)) {
        return udt(id, start, attribs);
    }

    if (accept(TokenKind::As)) {
        return alias(id, start, attribs);
    }

    return makeError(Diag::unexpectedToken, "'=' or end of statement", m_token.description());
}

/**
 *  alias
 *    = TypeExpr
 *    .
 */
Result<AstTypeAlias*> Parser::alias(llvm::StringRef id, llvm::SMLoc start, AstAttributeList* attribs) {
    TRY_DECLARE(type, typeExpr({ .typeOfAllowsExpr = true }))

    return m_context.create<AstTypeAlias>(
        llvm::SMRange{ start, m_endLoc },
        id,
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
Result<AstUdtDecl*> Parser::udt(llvm::StringRef id, llvm::SMLoc start, AstAttributeList* attribs) {
    // assume m_token == declaration || "end"
    TRY_DECLARE(decls, udtDeclList())

    TRY(consume(TokenKind::End))
    TRY(consume(TokenKind::Type))

    return m_context.create<AstUdtDecl>(
        llvm::SMRange{ start, m_endLoc },
        id,
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
        TRY_DECLARE(attribs, attributeList())
        if (attribs != nullptr) {
            TRY(expect(TokenKind::Identifier))
        } else if (m_token.isNot(TokenKind::Identifier)) {
            break;
        }

        TRY_DECLARE(member, udtMember(attribs))
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
    advance();

    TRY(consume(TokenKind::As))

    TRY_DECLARE(type, typeExpr({ .typeOfAllowsExpr = true }))

    return m_context.create<AstVarDecl>(
        llvm::SMRange{ start, m_endLoc },
        id,
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

    bool isFunction = m_token.is(TokenKind::Function);
    auto start = attribs != nullptr ? attribs->range.Start : m_token.range().Start;
    TRY_DECLARE(decl, funcSignature(start, attribs, {}))

    RESTORE_ON_EXIT(m_scope);
    m_scope = Scope::Function;
    AstStmtList* stmts = nullptr;

    if (accept(TokenKind::LambdaBody)) {
        AstStmt* stmt = nullptr;
        if (isFunction) {
            TRY_DECLARE(expr, expression())
            stmt = m_context.create<AstReturnStmt>(
                llvm::SMRange{ start, m_endLoc },
                expr);
        } else {
            TRY_ASSIGN(stmt, statement())
        }
        stmts = m_context.create<AstStmtList>(
            llvm::SMRange{ start, m_endLoc },
            std::vector<AstDecl*>{}, // TODO: Fix. stmt could be a declaration!
            std::vector<AstFuncStmt*>{},
            std::vector<AstStmt*>{ stmt });
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

    AstExpr* expr = nullptr;
    if (m_token.isNot(TokenKind::EndOfStmt)) {
        TRY_ASSIGN(expr, expression())
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
    TRY_DECLARE(block, ifBlock())
    blocks.emplace_back(block);

    if (m_token.is(TokenKind::EndOfStmt)) {
        Token next;
        m_source.peek(next);
        if (next.getKind() == TokenKind::Else) {
            advance();
        }
    }

    while (accept(TokenKind::Else)) {
        if (accept(TokenKind::If)) {
            TRY_DECLARE(if_, ifBlock())
            blocks.emplace_back(if_);
        } else {
            TRY_DECLARE(else_, thenBlock({}, nullptr))
            blocks.emplace_back(else_);
        }

        if (m_token.is(TokenKind::EndOfStmt)) {
            Token next;
            m_source.peek(next);
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
        TRY_DECLARE(var, kwDim(nullptr))
        decls.emplace_back(var);
        TRY(consume(TokenKind::Comma))
    }

    TRY_DECLARE(expr, expression({ .commaAsAnd = true }))
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
        TRY_DECLARE(var, kwDim(nullptr))
        decls.emplace_back(var);
        TRY(consume(TokenKind::Comma))
    }

    // id [ "AS" TypeExpr ] "=" Expression
    auto idStart = m_token.range().Start;
    TRY(expect(TokenKind::Identifier))
    auto id = m_token.getStringValue();
    advance();

    AstTypeExpr* type = nullptr;
    if (accept(TokenKind::As)) {
        TRY_ASSIGN(type, typeExpr())
    }

    TRY(consume(TokenKind::Assign))

    TRY_DECLARE(expr, expression())
    auto* iterator = m_context.create<AstVarDecl>(
        llvm::SMRange{ idStart, m_endLoc },
        id,
        nullptr,
        type,
        expr);

    // "TO" Expression [ "STEP" expression ]
    TRY(consume(TokenKind::To))

    TRY_DECLARE(limit, expression())
    AstExpr* step = nullptr;
    if (accept(TokenKind::Step)) {
        TRY_ASSIGN(step, expression())
    }

    // "=>" statement ?
    AstStmt* stmt = nullptr;
    llvm::StringRef next;
    if (accept(TokenKind::LambdaBody)) {
        TRY_ASSIGN(stmt, statement())
    } else {
        TRY(consume(TokenKind::EndOfStmt))
        TRY_ASSIGN(stmt, stmtList())
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
    AstStmt* stmt = nullptr;
    AstExpr* expr = nullptr;
    std::vector<AstVarDecl*> decls;

    // [ VAR { "," VAR } ]
    auto acceptComma = false;
    while (m_token.is(TokenKind::Dim) || (acceptComma && accept(TokenKind::Comma))) {
        acceptComma = true;
        TRY_DECLARE(var, kwDim(nullptr))
        decls.emplace_back(var);
    }

    // ( EoS StmtList "LOOP" [ Condition ]
    if (accept(TokenKind::EndOfStmt)) {
        TRY_ASSIGN(stmt, stmtList())
        TRY(consume(TokenKind::Loop))

        // [ Condition ]
        if (accept(TokenKind::Until)) {
            condition = AstDoLoopStmt::Condition::PostUntil;
            TRY_ASSIGN(expr, expression({ .commaAsAnd = true }))
        } else if (accept(TokenKind::While)) {
            condition = AstDoLoopStmt::Condition::PostWhile;
            TRY_ASSIGN(expr, expression({ .commaAsAnd = true }))
        }
    } else {
        // [ Condition ]
        if (accept(TokenKind::Until)) {
            condition = AstDoLoopStmt::Condition::PreUntil;
            TRY_ASSIGN(expr, expression({ .commaAsAnd = true }))
        } else if (accept(TokenKind::While)) {
            condition = AstDoLoopStmt::Condition::PreWhile;
            TRY_ASSIGN(expr, expression({ .commaAsAnd = true }))
        }

        // EoS StmtList "LOOP"
        if (accept(TokenKind::EndOfStmt)) {
            TRY_ASSIGN(stmt, stmtList())
            TRY(consume(TokenKind::Loop))
        }
        // "=>" Statement
        else {
            TRY(consume(TokenKind::LambdaBody))
            TRY_ASSIGN(stmt, statement())
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
Result<AstTypeExpr*> Parser::typeExpr(TypeFlags flags) {
    RESTORE_ON_EXIT(m_typeFlags);
    m_typeFlags = flags;

    auto start = m_token.range().Start;
    bool parenthesized = accept(TokenKind::ParenOpen);
    bool mustBePtr = false;

    AstTypeExpr::TypeExpr expr;
    if (m_token.isOneOf(TokenKind::Sub, TokenKind::Function)) {
        TRY_ASSIGN(expr, funcSignature(start, nullptr, { .isAnonymous = true }))
        mustBePtr = true;
    } else if (m_token.is(TokenKind::Any) || m_token.isTypeKeyword()) {
        expr = m_token.getKind();
        advance();
    } else if (m_token.is(TokenKind::TypeOf)) {
        TRY_ASSIGN(expr, kwTypeOf())
    } else {
        TRY_DECLARE(ident, identifier())
        if (m_symbolTable != nullptr) {
            auto* symbol = m_symbolTable->find(ident->name);
            if (symbol == nullptr || symbol->valueFlags().kind != ValueFlags::Kind::type) {
                return Result<AstTypeExpr*>::makeError();
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
        return Result<AstTypeExpr*>::makeError();
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
    std::vector<Token> tokens;
    int parens = 1;
    while (true) {
        if (m_token.isOneOf(TokenKind::EndOfStmt, TokenKind::EndOfFile, TokenKind::Invalid)) {
            return makeError(Diag::unexpectedToken, "type expression", m_token.description());
        }
        if (m_token.is(TokenKind::ParenClose)) {
            parens--;
            if (parens == 0) {
                break;
            }
            if (parens < 0) {
                return makeError(Diag::unexpectedToken, "type expression", m_token.description());
            }
        } else if (m_token.is(TokenKind::ParenOpen)) {
            parens++;
        }
        tokens.emplace_back(m_token);
        advance();
    }
    if (tokens.empty()) {
        return makeError(Diag::unexpectedToken, "type expression", m_token.description());
    }
    TRY(consume(TokenKind::ParenClose))

    bool typeOfAllowsExpr = m_typeFlags.typeOfAllowsExpr;
    return m_context.create<AstTypeOf>(llvm::SMRange{ start, m_endLoc }, std::move(tokens), typeOfAllowsExpr);
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
    TRY_DECLARE(expr, factor())

    fixExprOperators();

    // expr op
    if (m_token.isOperator()) {
        TRY_ASSIGN(expr, expression(expr, 1))
    }

    // print "hello"
    if (flags.callWithoutParens && llvm::isa<AstIdentExpr>(expr) && allowCallWithToken(m_token)) {
        auto start = expr->range.Start;
        TRY_DECLARE(args, expressionList())

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

        TRY_DECLARE(rhs, factor())
        fixExprOperators();

        while (m_token.getPrecedence() > current || (m_token.isRightToLeft() && m_token.getPrecedence() == current)) {
            TRY_ASSIGN(rhs, expression(rhs, m_token.getPrecedence()))
        }

        auto start = lhs->range.Start;
        TRY_ASSIGN(lhs, binary({ start, m_endLoc }, kind, lhs, rhs))
    }
    return lhs;
}

void Parser::fixExprOperators() noexcept {
    if (m_token.is(TokenKind::Assign)) {
        if (m_exprFlags.useAssign) {
            m_exprFlags.useAssign = false;
        } else {
            m_token.setKind(TokenKind::Equal);
        }
    } else if (m_token.is(TokenKind::Comma) && m_exprFlags.commaAsAnd) {
        m_token.setKind(TokenKind::CommaAnd);
    }
}

/**
 * factor = primary { <Right Unary Op> | "AS" TypeExpr } .
 */
Result<AstExpr*> Parser::factor() {
    auto start = m_token.range().Start;
    TRY_DECLARE(expr, primary())

    while (true) {
        // <Right Unary Op>
        if (m_token.isUnary() && m_token.isRightToLeft()) {
            auto kind = m_token.getKind();
            advance();
            TRY_ASSIGN(expr, unary({ start, m_endLoc }, kind, expr))
            continue;
        }

        if (accept(TokenKind::ParenOpen)) {
            TRY_DECLARE(args, expressionList())
            TRY(consume(TokenKind::ParenClose))
            expr = m_context.create<AstCallExpr>(
                llvm::SMRange{ start, m_endLoc },
                expr,
                args);
            continue;
        }

        // "AS" TypeExpr
        if (accept(TokenKind::As)) {
            TRY_DECLARE(type, typeExpr({ .typeOfAllowsExpr = true }))
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
        TRY_DECLARE(expr, expression())
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

        TRY_DECLARE(fact, factor())
        TRY_DECLARE(expr, expression(fact, prec))

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
    TRY_DECLARE(id, identifier())

    TRY(consume(TokenKind::ParenOpen))
    TRY_DECLARE(args, expressionList())
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

    TRY_DECLARE(expr, expression({ .commaAsAnd = true }))

    TRY(consume(TokenKind::Then))
    TRY_DECLARE(trueExpr, expression())

    TRY(consume(TokenKind::Else))
    TRY_DECLARE(falseExpr, expression())

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
    auto value = m_token.getValue();
    advance();

    return m_context.create<AstLiteralExpr>(
        m_token.range(),
        value);
}

/**
 * Parse comma separated list of expressionds
 */
Result<AstExprList*> Parser::expressionList() {
    auto start = m_token.range().Start;
    std::vector<AstExpr*> exprs;

    while (!m_token.isOneOf(TokenKind::EndOfFile, TokenKind::ParenClose, TokenKind::EndOfStmt)) {
        TRY_DECLARE(expr, expression())
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

void Parser::replace(TokenKind what, TokenKind with) noexcept {
    if (m_token.is(what)) {
        m_token.setKind(with);
    }
}

[[nodiscard]] Result<> Parser::expect(TokenKind kind) const noexcept {
    if (m_token.is(kind)) {
        return {};
    }

    return makeError(Diag::unexpectedToken, Token::description(kind), m_token.description());
}

void Parser::advance() {
    m_endLoc = m_token.range().End;
    m_source.next(m_token);
}
