//
// Created by Albert Varaksin on 14/02/2026.
//
#include "Parser.hpp"
using namespace lbc;

/// declaration = ... .
auto Parser::declaration() -> Result<void> {
    (void)this;
    return notImplemented();
}

/**
 * varDecl = id ( "AS" typeExpr [ "=" expression ]
 *              | "=" expression
 *              ) .
 */
auto Parser::varDecl() -> Result<AstVarDecl*> {
    const auto start = startLoc();
    TRY_DECL(id, identifier())

    // "AS" typeExpr
    TRY_IF(accept(TokenKind::As)) {
        return notImplemented();
    }

    // "=" expr
    TRY(consume(TokenKind::Assign))
    TRY_DECL(expr, expression())

    return make<AstVarDecl>(range(start), id, nullptr, expr);
}
