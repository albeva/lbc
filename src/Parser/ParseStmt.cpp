//
// Created by Albert Varaksin on 14/02/2026.
//
#include "Ast/Ast.hpp"
#include "Parser.hpp"
using namespace lbc;

auto Parser::stmtList() -> Result<AstStmtList*> {
    const auto start = m_token.getRange().Start;
    Sequencer<AstDecl> decls {};
    Sequencer<AstStmt> stmts {};
    // TODO: iterate over statements
    return make<AstStmtList>(range(start), sequence(decls), sequence(stmts));
}

auto Parser::statement() -> Result<void> {
    (void)this;
    return notImplemented();
}
