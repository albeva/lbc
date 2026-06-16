//
// Created by Albert Varaksin on 14/02/2026.
//
#include "Ast/Ast.hpp"
#include "Ast/AstVisitor.hpp"
#include "Parser.hpp"
using namespace lbc;

namespace {
/// Check if given token should terminate a statement list.
auto isTerminator(const Token& tkn) -> bool {
    return tkn.kind().isOneOf(TokenKind::Invalid, TokenKind::EndOfFile, TokenKind::End);
}
} // namespace

/// stmtList = { statement EOS } .
auto Parser::stmtList() -> Result<AstStmtList*> {
    const auto start = m_token.getRange().Start;
    Sequencer<AstDecl> decls {};
    Sequencer<AstStmt> stmts {};

    // { Statement EOS } .
    while (not isTerminator(m_token)) {
        TRY_DECL(stmt, statement())
        TRY(consume(TokenKind::EndOfStmt))
        stmts.add(stmt);
        const auto visitor = Visitor {
            [&](const AstDimStmt& ast) {
                decls.append(ast.getDecls());
            },
            [&](const AstDeclareStmt& ast) {
                decls.add(ast.getDecl());
            },
            [&](const AstFuncStmt& ast) {
                decls.add(ast.getDecl());
            },
            [](const auto&) {}
        };
        visit(*stmt, visitor);
    }

    return make<AstStmtList>(range(start), sequence(decls), sequence(stmts));
}

/**
 * statement = declareStmt
 *           | externStmt
 *           | funcStmt
 *           | returnStmt
 *           | dimStmt
 *           | ( expression [ "=" expression ] )
 *           .
 */
auto Parser::statement() -> Result<AstStmt*> {
    switch (m_token.kind().value()) {
    case TokenKind::Dim:
        return dimStmt();
    case TokenKind::Declare:
        return declareStmt();
    case TokenKind::Sub:
    case TokenKind::Function:
        return funcStmt();
    case TokenKind::Return:
        return returnStmt();
    case TokenKind::Extern:
        return externStmt();
    default:
        TRY_DECL(primary, expression({ .callWithoutParens = true, .stopAtAssign = true }));
        TRY_IF (accept(TokenKind::Assign)) {
            TRY_DECL(expr, expression())
            return make<AstAssignStmt>(range(primary, expr), primary, expr);
        }
        return make<AstExprStmt>(primary->getRange(), primary);
    }
}

/// dimStmt = "DIM" varDecl { "," varDecl } .
auto Parser::dimStmt() -> Result<AstStmt*> {
    const auto start = startLoc();
    TRY(consume(TokenKind::Dim))

    // varDecl { "," varDecl }
    Sequencer<AstVarDecl> decls {};
    TRY_ADD(decls, varDecl())
    TRY_WHILE (accept(TokenKind::Comma)) {
        TRY_ADD(decls, varDecl())
    }

    return make<AstDimStmt>(range(start), sequence(decls));
}

/// declareStmt = "DECLARE" ( subDecl | funcDecl )
auto Parser::declareStmt() -> Result<AstStmt*> {
    const auto start = startLoc();
    TRY(consume(TokenKind::Declare))

    AstFuncDecl* decl {}; // NOLINT(*-const-correctness)
    switch (m_token.kind().value()) {
    case TokenKind::Sub:
        TRY_ASSIGN(decl, subDecl())
        break;
    case TokenKind::Function:
        TRY_ASSIGN(decl, funcDecl())
        break;
    default:
        return unexpected();
    }

    return make<AstDeclareStmt>(range(start), decl);
}

/**
 * funcStmt = ( subDecl | funcDecl ) EOS stmtList "END" ( "SUB" | "FUNCTION" ) .
 *
 * Reuses the declaration parsers for the header, parses the body up to the
 * matching END SUB / END FUNCTION, and links the definition onto its decl.
 */
auto Parser::funcStmt() -> Result<AstStmt*> {
    const auto start = startLoc();
    const bool isSub = m_token.kind() == TokenKind::Sub;

    AstFuncDecl* decl {}; // NOLINT(*-const-correctness)
    if (isSub) {
        TRY_ASSIGN(decl, subDecl())
    } else {
        TRY_ASSIGN(decl, funcDecl())
    }

    // EOS stmtList "END" ( "SUB" | "FUNCTION" )
    TRY(consume(TokenKind::EndOfStmt))
    TRY_DECL(body, stmtList())
    TRY(consume(TokenKind::End))
    TRY(consume(isSub ? TokenKind::Sub : TokenKind::Function))

    auto* stmt = make<AstFuncStmt>(range(start), decl, body);
    decl->setImpl(stmt);
    return stmt;
}

/**
 * returnStmt = "RETURN" [ expression ] .
 *
 * A bare RETURN is terminated by the end-of-statement; the function-vs-subroutine
 * return rules are enforced during semantic analysis.
 */
auto Parser::returnStmt() -> Result<AstStmt*> {
    const auto start = startLoc();
    TRY(consume(TokenKind::Return))

    AstExpr* expr {}; // NOLINT(*-const-correctness)
    if (m_token.kind() != TokenKind::EndOfStmt) {
        TRY_ASSIGN(expr, expression())
    }
    return make<AstReturnStmt>(range(start), expr);
}

/**
 * externStmt = "EXTERN" <string>
 *              ( statement
 *              | EOS { statement EOS } "END" "EXTERN"
 *              ) .
 *
 * Stores the raw language string and the contained statements verbatim. The
 * linkage is validated and the symbol aliasing is resolved during sema.
 */
auto Parser::externStmt() -> Result<AstStmt*> {
    const auto start = startLoc();
    TRY(consume(TokenKind::Extern))

    // Language linkage string → ExternKind (only "C" is supported).
    TRY(expect(TokenKind::StringLiteral))
    const auto language = m_token.getValue().get<llvm::StringRef>();
    if (not language.equals_insensitive("C")) {
        return diag(diagnostics::unsupportedLinkage(language), m_token.getRange());
    }
    TRY(advance())

    // For now only DECLARE statements are accepted inside; the AstStmt* element
    // type leaves room for extern variables and other declarations later.
    Sequencer<AstStmt> stmts {};
    if (m_token.kind() == TokenKind::EndOfStmt) {
        // Block form: EXTERN "C" <EOS> { declareStmt <EOS> } END EXTERN
        TRY(consume(TokenKind::EndOfStmt))
        while (m_token.kind() != TokenKind::End) {
            TRY_ADD(stmts, declareStmt())
            TRY(consume(TokenKind::EndOfStmt))
        }
        TRY(consume(TokenKind::End))
        TRY(consume(TokenKind::Extern))
    } else {
        // Single-line form: EXTERN "C" declareStmt
        TRY_ADD(stmts, declareStmt())
    }

    return make<AstExtern>(range(start), ExternKind::C, sequence(stmts));
}
