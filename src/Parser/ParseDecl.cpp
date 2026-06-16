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
    // Record the verbatim name (before upper-casing) for an EXTERN "C" alias.
    const auto sourceName = getContext().retain(m_token.lexeme());
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

    auto* decl = make<AstVarDecl>(range(start), id, ty, expr);
    decl->setSourceName(sourceName);
    return decl;
}

// subDecl = "SUB" [ "(" params ")" ] .
auto Parser::subDecl() -> Result<AstFuncDecl*> {
    const auto start = startLoc();
    TRY(consume(TokenKind::Sub))
    // Record the verbatim name (before upper-casing); sema uses it as the
    // symbol alias for EXTERN "C" declarations.
    const auto sourceName = getContext().retain(m_token.lexeme());
    TRY_DECL(id, identifier())

    bool variadic = false;
    TRY_DECL(params, paramList(false, variadic));

    auto* decl = make<AstFuncDecl>(range(start), id, params, nullptr);
    decl->setSourceName(sourceName);
    decl->setVariadic(variadic);
    return decl;
}

// funcDecl = "FUNCTION" "(" [ params ] ")" "AS" type .
auto Parser::funcDecl() -> Result<AstFuncDecl*> {
    const auto start = startLoc();
    TRY(consume(TokenKind::Function))
    // Record the verbatim name (before upper-casing); sema uses it as the
    // symbol alias for EXTERN "C" declarations.
    const auto sourceName = getContext().retain(m_token.lexeme());
    TRY_DECL(id, identifier())

    bool variadic = false;
    TRY_DECL(params, paramList(true, variadic));

    TRY(consume(TokenKind::As))
    TRY_DECL(ty, type())
    auto* decl = make<AstFuncDecl>(range(start), id, params, ty);
    decl->setSourceName(sourceName);
    decl->setVariadic(variadic);
    return decl;
}

// paramList = ( param | "..." ) { "," ( param | "..." ) } .
// A "..." marks a C-style variadic and must be the last entry — anything after it
// fails the caller's ")" expectation. Whether "..." is permitted at all (extern "C"
// only) is enforced during semantic analysis.
auto Parser::paramList(const bool requireParens, bool& variadic) -> Result<std::span<AstFuncParamDecl*>> {
    // Opening "(": required for functions; for subs its absence just means no
    // parameters. (accept/expect return DiagResult<bool>/<void>, so their result
    // must be unwrapped — a raw `if (accept(...))` would test the error state.)
    if (requireParens) {
        TRY(consume(TokenKind::ParenOpen))
    } else {
        TRY_IF_NOT(accept(TokenKind::ParenOpen)) {
            return {};
        }
    }

    // Empty parameter list "()".
    TRY_IF(accept(TokenKind::ParenClose)) {
        return {};
    }

    // ( param | "..." ) { "," ( param | "..." ) } — a "..." ends the list.
    Sequencer<AstFuncParamDecl> params {};
    while (true) {
        TRY_IF(accept(TokenKind::Ellipsis)) {
            variadic = true;
            break;
        }
        TRY_ADD(params, paramDecl())
        TRY_IF_NOT(accept(TokenKind::Comma)) {
            break;
        }
    }

    TRY(consume(TokenKind::ParenClose))
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
