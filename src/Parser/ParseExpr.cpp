//
// Created by Albert Varaksin on 14/02/2026.
//
// Expression parser.
//
// Uses precedence climbing (a variant of Pratt parsing) to handle binary
// operator precedence and associativity without separate functions per
// precedence level. The entry point is expression(), which parses a
// primary and then calls climb() to consume any following binary and
// suffix operators.
//
// BASIC-specific concerns handled here:
//   - Assignment (`=`) is a statement, not an expression. When stopAtAssign
//     is set, the climber stops before consuming `=` so the statement-level
//     parser can handle it.
//   - Subroutine calls may omit parentheses: `Print x, y`. When
//     callWithoutParens is set and the primary is a bare identifier
//     followed by a non-binary token, it is parsed as a paren-free call.
//   - `-` and `*` are rewritten to Negate/Dereference in primary context
//     so the same token kinds can serve as both binary and unary operators.
//
#include "Parser.hpp"
using namespace lbc;

// expression = primary { <binary-op> primary } .
auto Parser::expression(const ExprFlags flags) -> Result<AstExpr*> {
    const ValueRestorer restorer { m_exprFlags };
    m_exprFlags = flags;

    TRY_DECL(lhs, primary())

    if (shouldBreak()) {
        return lhs;
    }

    // Enable function call without parentheses
    if (flags.callWithoutParens && llvm::isa<AstVarExpr>(lhs)) {
        if (not(m_token.kind().isBinary() && m_token.kind().isLeftAssociative())) {
            return sub(lhs);
        }
    }

    return climb(lhs);
}

/**
 * primary = variable
 *         | literal
 *         | "(" expression ")"
 *         | prefix
 *         .
 */
auto Parser::primary() -> Result<AstExpr*> {
    switch (m_token.kind().value()) {
    case TokenKind::Identifier:
        return variable();
    case TokenKind::BooleanLiteral:
    case TokenKind::IntegerLiteral:
    case TokenKind::FloatLiteral:
    case TokenKind::StringLiteral:
    case TokenKind::NullLiteral:
        return literal();
    case TokenKind::ParenOpen: {
        TRY(advance());
        TRY_DECL(expr, expression())
        TRY(consume(TokenKind::ParenClose))
        return expr;
    }
    case TokenKind::Minus:
        m_token.changeKind(TokenKind::Negate);
        return prefix();
    case TokenKind::Multiply:
        m_token.changeKind(TokenKind::Dereference);
        return prefix();
    default:
        return prefix();
    }
}

/// variable = id .
auto Parser::variable() -> Result<AstExpr*> {
    const auto start = startLoc();
    TRY_DECL(id, identifier());
    return make<AstVarExpr>(range(start), id);
}

/**
 * literal = "null"
 *         | "true" | "false"
 *         | <integer> | <float>
 *         | <string>
 *         .
 */
auto Parser::literal() -> Result<AstExpr*> {
    auto* expr = make<AstLiteralExpr>(m_token.getRange(), m_token.getValue());
    TRY(advance())
    return expr;
}

/// sub = callee [ params ] .
auto Parser::sub(AstExpr* callee) -> Result<AstExpr*> {
    std::span<AstExpr*> exprs;
    if (m_token.kind() != TokenKind::EndOfStmt) {
        TRY_ASSIGN(exprs, params())
    }
    return make<AstCallExpr>(range(callee), callee, exprs);
}

/// function = callee "(" [ params ] ")" .
auto Parser::function(AstExpr* callee) -> Result<AstExpr*> {
    TRY(consume(TokenKind::ParenOpen))
    std::span<AstExpr*> exprs;
    TRY_IF_NOT(accept(TokenKind::ParenClose)) {
        TRY_ASSIGN(exprs, params())
        TRY(consume(TokenKind::ParenClose))
    }
    return make<AstCallExpr>(range(callee), callee, exprs);
}

/// params = expression { "," expression } .
auto Parser::params() -> Result<std::span<AstExpr*>> {
    Sequencer<AstExpr> args {};
    TRY_ADD(args, expression())
    TRY_WHILE(accept(TokenKind::Comma)) {
        TRY_ADD(args, expression())
    }
    return sequence(args);
}

/// prefix = <unary-op> primary .
auto Parser::prefix() -> Result<AstExpr*> {
    if (!m_token.kind().isUnary()) {
        return expected("unary expression");
    }
    const auto start = startLoc();
    const auto kind = m_token.kind();
    TRY(advance())

    TRY_DECL(lhs, primary())
    TRY_DECL(expr, climb(lhs, kind.getPrecedence()))
    return make<AstUnaryExpr>(range(start), expr, kind);
}

/**
 * Parse a suffix operator applied to @param lhs.
 * Dispatches to the appropriate handler based on the current token.
 */
auto Parser::suffix(AstExpr* lhs) -> Result<AstExpr*> {
    switch (m_token.kind().value()) {
    case TokenKind::Value::ParenOpen:
        return function(lhs);
    case TokenKind::Value::As:
    case TokenKind::Value::Is:
        return notImplemented();
    default:
        std::unreachable();
    }
}

/**
 * Construct the appropriate binary AST node for the given operator.
 * Handles special cases like short-circuit `AND` and member access.
 */
auto Parser::binary(AstExpr* lhs, AstExpr* rhs, const TokenKind tkn) -> Result<AstExpr*> {
    const auto loc = range(lhs, rhs);
    switch (tkn.value()) {
    case TokenKind::Value::ConditionAnd:
        return make<AstBinaryExpr>(loc, lhs, rhs, TokenKind::LogicalAnd);
    case TokenKind::Value::MemberAccess:
        return make<AstMemberExpr>(loc, lhs, rhs, tkn);
    default:
        return make<AstBinaryExpr>(loc, lhs, rhs, tkn);
    }
}

/**
 * Precedence-climbing loop. Consumes binary and suffix operators
 * at or above @param precedence, building the AST bottom-up.
 */
auto Parser::climb(AstExpr* lhs, const int precedence) -> Result<AstExpr*> {
    if (shouldBreak()) {
        return lhs;
    }

    const auto getKind = [&] -> TokenKind {
        if (m_token.kind() == TokenKind::Assign) {
            m_token.changeKind(TokenKind::Equal);
        }
        return m_token.kind();
    };

    auto kind = getKind();
    while (kind.getPrecedence() >= precedence) {
        if (kind.isUnary()) {
            TRY_ASSIGN(lhs, suffix(lhs));
            if (shouldBreak()) {
                break;
            }
            kind = getKind();
            continue;
        }

        const auto op = kind;
        TRY(advance());
        TRY_DECL(rhs, primary());
        if (shouldBreak()) {
            TRY_ASSIGN(lhs, binary(lhs, rhs, op));
            break;
        }
        kind = getKind();

        while (kind.getPrecedence() > op.getPrecedence() || (kind.isRightAssociative() && kind.getPrecedence() == op.getPrecedence())) {
            TRY_ASSIGN(rhs, climb(rhs, kind.getPrecedence()));
            kind = getKind();
        }
        TRY_ASSIGN(lhs, binary(lhs, rhs, op));
    }
    return lhs;
}

/**
 * Check if expression parsing should stop before the current token.
 * In BASIC, assignment is a statement-level construct, so the expression
 * parser must yield before consuming `=` when stopAtAssign is set.
 */
auto Parser::shouldBreak() const -> bool {
    return m_exprFlags.stopAtAssign && m_token.kind() == TokenKind::Assign;
}
