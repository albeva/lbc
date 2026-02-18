//
// Created by Albert Varaksin on 14/02/2026.
//
#include "Ast/Ast.hpp"
#include "Parser.hpp"
#include "Ast/AstVisitor.hpp"
using namespace lbc;

namespace {
/// Check if given token should terminate statement list
auto isTerminator(const Token& tkn) -> bool {
    return tkn.kind().isOneOf(TokenKind::Invalid, TokenKind::EndOfFile, TokenKind::End);
}
} // namespace

// stmtList = { statement EOS } .
auto Parser::stmtList() -> Result<AstStmtList*> {
    const auto start = m_token.getRange().Start;
    Sequencer<AstDecl> decls {};
    Sequencer<AstStmt> stmts {};

    // { Statement EOS } .
    while (not isTerminator(m_token)) {
        TRY_DECL(stmt, statement())
        TRY(consume(TokenKind::EndOfStmt))
        stmts.add(stmt);
        // collection declarations
        visit(*stmt, Visitor {
            [&](const AstDimStmt& ast) {
                decls.append(ast.getDecls());
            },
            [&](const AstDeclareStmt& ast) {
                decls.add(ast.getDecl());
            },
            [](const auto&){}
        });
    }

    return make<AstStmtList>(range(start), sequence(decls), sequence(stmts));
}

// statement = .
auto Parser::statement() -> Result<AstStmt*> {
    (void)this;
    return notImplemented();
}
