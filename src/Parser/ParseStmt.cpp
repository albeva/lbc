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
            [](const auto&) {}
        };
        visit(*stmt, visitor);
    }

    return make<AstStmtList>(range(start), sequence(decls), sequence(stmts));
}

/**
 * statement = declareStmt
 *           | dimStmt
 *           .
 */
auto Parser::statement() -> Result<AstStmt*> {
    switch (m_token.kind().value()) {
    case TokenKind::Dim:
        return dimStmt();
    case TokenKind::Declare:
        return declareStmt();
    default:
        return unexpected();
    }
}

/// dimStmt = "DIM" varDecl { "," varDecl } .
auto Parser::dimStmt() -> Result<AstStmt*> {
    const auto start = startLoc();
    TRY(consume(TokenKind::Dim))

    // varDecl { "," varDecl }
    Sequencer<AstVarDecl> decls {};
    TRY_ADD(decls, varDecl())
    TRY_WHILE(accept(TokenKind::Comma)) {
        TRY_ADD(decls, varDecl())
    }

    return make<AstDimStmt>(range(start), sequence(decls));
}

/// declareStmt = ... .
auto Parser::declareStmt() -> Result<AstStmt*> {
    return notImplemented();
}
