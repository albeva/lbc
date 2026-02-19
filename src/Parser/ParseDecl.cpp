//
// Created by Albert Varaksin on 14/02/2026.
//
#include "Parser.hpp"
using namespace lbc;

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
    TRY_IF (accept(TokenKind::As)) {
        TRY_ASSIGN(ty, type())
        TRY_IF (accept(TokenKind::Assign)) {
            TRY_ASSIGN(expr, expression())
        }
    } else {
        TRY(consume(TokenKind::Assign))
        TRY_ASSIGN(expr, expression())
    }

    return make<AstVarDecl>(range(start), id, ty, expr);
}

// subDecl = "SUB" [ "(" params ")" ] .
auto Parser::subDecl() -> Result<AstFuncDecl*> {
    const auto start = startLoc();
    TRY(consume(TokenKind::Sub))
    TRY_DECL(id, identifier())

    std::span<AstFuncParamDecl*> params;
    TRY_IF (accept(TokenKind::ParenOpen)) {
        TRY_ASSIGN(params, paramList())
        TRY(consume(TokenKind::ParenClose))
    }
    return make<AstFuncDecl>(range(start), id, params, nullptr);
}

// funcDecl = "FUNCTION" "(" [ params ] ")" "AS" type .
auto Parser::funcDecl() -> Result<AstFuncDecl*> {
    const auto start = startLoc();
    TRY(consume(TokenKind::Function))
    TRY_DECL(id, identifier())

    TRY(consume(TokenKind::ParenOpen))
    std::span<AstFuncParamDecl*> params;
    TRY_IF_NOT (accept(TokenKind::ParenClose)) {
        TRY_ASSIGN(params, paramList())
        TRY(consume(TokenKind::ParenClose))
    }

    TRY(consume(TokenKind::As))
    TRY_DECL(ty, type())
    return make<AstFuncDecl>(range(start), id, params, ty);
}

// paramList = param { "," param } .
auto Parser::paramList() -> Result<std::span<AstFuncParamDecl*>> {
    Sequencer<AstFuncParamDecl> params {};
    TRY_ADD(params, paramDecl())
    TRY_WHILE (accept(TokenKind::Comma)) {
        TRY_ADD(params, paramDecl())
    }
    return sequence(params);
}

// param = identifier "AS" type .
auto Parser::paramDecl() -> Result<AstFuncParamDecl*> {
    const auto start = startLoc();
    TRY_DECL(id, identifier())
    TRY(consume(TokenKind::As))
    TRY_DECL(ty, type())
    return make<AstFuncParamDecl>(range(start), id, ty);
}
