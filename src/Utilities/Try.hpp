//
// Created by Albert Varaksin on 12/02/2026.
//
// Macros to simplify error std::expected / std::unexpected handling
// This is inspired by SerenityOS / Ladybird browser
#pragma once

#define TRY_CONCAT_IMPL(x, y) x##y
#define TRY_CONCAT(x, y) TRY_CONCAT_IMPL(x, y)
#define TRY_RESULT(x) TRY_CONCAT(x, __COUNTER__)

/**
 * Evaluate a `std::expected` expression and propagate the error on failure.
 *
 * @param ... Expression returning `std::expected`.
 *
 * @code
 * TRY(consume(TokenKind::Comma))
 * @endcode
 */
#define TRY(...) TRY_IMPL(TRY_RESULT(try_), __VA_ARGS__)
#define TRY_IMPL(res, expr)                             \
    if (auto&& res = (expr); not res) {                 \
        return std::unexpected(std::move(res.error())); \
    }

/**
 * Evaluate a `std::expected<bool>` expression, propagate on error,
 * and execute the following statement/block when the value is `true`.
 *
 * @param ... Expression returning `std::expected<bool>`.
 *
 * @code
 * TRY_IF(accept(TokenKind::As)) {
 *     TRY_ASSIGN(typeExpr, type())
 * }
 * @endcode
 */
#define TRY_IF(...) TRY_IF_IMPL(TRY_RESULT(try_if_), __VA_ARGS__)
#define TRY_IF_IMPL(res, ...)                           \
    if (auto&& res = (__VA_ARGS__); not res) {          \
        return std::unexpected(std::move(res.error())); \
    } else if (res.value())

/**
 * Evaluate a `std::expected<bool>` expression, propagate on error,
 * and execute the following statement/block when the value is `false`.
 *
 * @param ... Expression returning `std::expected<bool>`.
 *
 * @code
 * TRY_IF_NOT(accept(TokenKind::EndOfStmt)) {
 *     TRY_ASSIGN(typeExpr, type())
 * }
 * @endcode
 */
#define TRY_IF_NOT(...) TRY_IF_NOT_IMPL(TRY_RESULT(try_if_), __VA_ARGS__)
#define TRY_IF_NOT_IMPL(res, ...)                       \
    if (auto&& res = (__VA_ARGS__); not res) {          \
        return std::unexpected(std::move(res.error())); \
    } else if (not res.value())

/**
 * Evaluate a `std::expected<bool>` expression in a loop, propagate on error,
 * and execute the following statement/block while the value is `true`.
 *
 * @param ... Expression returning `std::expected<bool>`.
 *
 * @code
 * TRY_WHILE(accept(TokenKind::Comma)) {
 *     TRY_ADD(args, expression())
 * }
 * @endcode
 */
#define TRY_WHILE(...) TRY_WHILE_IMPL(TRY_RESULT(try_while_), __VA_ARGS__)
#define TRY_WHILE_IMPL(res, ...)                            \
    for (auto&& res = (__VA_ARGS__);; res = (__VA_ARGS__))  \
        if (not res) {                                      \
            return std::unexpected(std::move(res.error())); \
        } else if (not res.value()) {                       \
            break;                                          \
        } else

/**
 * Evaluate a `std::expected` expression and trigger a fatal error on failure.
 *
 * @param ... Expression returning `std::expected`.
 */
#define MUST(...) MUST_IMPL(TRY_RESULT(must_), __VA_ARGS__)
#define MUST_IMPL(res, ...)                  \
    if (auto res = (__VA_ARGS__); not res) { \
        std::unreachable();                  \
    }

/**
 * Evaluate a `std::expected` expression, propagate on error,
 * and assign the value to an existing variable on success.
 *
 * @param var Variable to assign the value to.
 * @param ... Expression returning `std::expected`.
 *
 * @code
 * AstExpr* expr {};
 * TRY_ASSIGN(expr, expression())
 * @endcode
 */
#define TRY_ASSIGN(var, ...) TRY_ASSIGN_IMPL(TRY_RESULT(try_assign_), var, __VA_ARGS__)
#define TRY_ASSIGN_IMPL(res, var, ...)                  \
    if (auto&& res = (__VA_ARGS__); not res) {          \
        return std::unexpected(std::move(res.error())); \
    } else {                                            \
        (var) = std::move(res.value());                 \
    }

/**
 * Evaluate a `std::expected` expression, propagate on error,
 * and declare a new variable initialized with the value on success.
 *
 * @param var Variable to declare.
 * @param ... Expression returning `std::expected`.
 *
 * @code
 * TRY_DECL(expr, expression())
 * @endcode
 */
#define TRY_DECL(var, ...)                                    \
    std::remove_cvref_t<decltype((__VA_ARGS__).value())> var; \
    TRY_ASSIGN(var, __VA_ARGS__);

/**
 * Evaluate a `std::expected` expression, propagate on error,
 * and append the value to a `Sequencer` on success.
 *
 * @param seq Sequencer to add the value to.
 * @param ... Expression returning `std::expected`.
 *
 * @code
 * Sequencer<AstVarDecl> decls {};
 * TRY_ADD(decls, varDecl())
 * @endcode
 */
#define TRY_ADD(seq, ...) TRY_ADD_IMPL(TRY_RESULT(try_add_), seq, __VA_ARGS__)
#define TRY_ADD_IMPL(res, seq, ...)                     \
    if (auto&& res = (__VA_ARGS__); not res) {          \
        return std::unexpected(std::move(res.error())); \
    } else {                                            \
        seq.add(std::move(res.value()));                \
    }
