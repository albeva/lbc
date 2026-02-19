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
    AstType* ty {};   // NOLINT(*-const-correctness)
    AstExpr* expr {}; // NOLINT(*-const-correctness)

    // "AS" typeExpr [ "=" expression
    TRY_IF(accept(TokenKind::As)) {
        TRY_ASSIGN(ty, type())
        TRY_IF(accept(TokenKind::Assign)) {
            TRY_ASSIGN(expr, expression())
        }
    }
    // "=" expression
    else {
        TRY(consume(TokenKind::Assign))
        TRY_ASSIGN(expr, expression())
    }

    return make<AstVarDecl>(range(start), id, ty, expr);
}
