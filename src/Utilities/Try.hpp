//
// Created by Albert Varaksin on 12/02/2026.
//
// Macros to simplify error std::expected / std::unexpected handling
// This is inspired by SerenityOS / Ladybird browser
#pragma once
#include "Diagnostics.hpp"

/**
 * Executes an expression returning `std::expected` and propagates errors.
 *
 * @param ... Expression which returns `std::expected` type.
 *
 * @code
 * TRY(consume(TokenKind::Comma()))
 * @endcode
 */
#define TRY(...)                                                             \
    {                                                                        \
        LBC_IGNORE_DIAGNOSTIC("-Wshadow", auto try_tmp_res = (__VA_ARGS__)); \
        if (!try_tmp_res) {                                                  \
            return std::unexpected(std::move(try_tmp_res.error()));          \
        }                                                                    \
    }

/**
 * A macro that checks if the given expression has an error.
 * If it does, it triggers a fatal error.
 *
 * @param ... Expression which returns `std::expected` type.
 */
#define MUST(...)                                                            \
    {                                                                        \
        LBC_IGNORE_DIAGNOSTIC("-Wshadow", auto try_tmp_res = (__VA_ARGS__)); \
        if (!try_tmp_res) {                                                  \
            std::unreachable();                                              \
        }                                                                    \
    }

/**
 * Executes an expression returning `std::expected`, on success assigns the
 * value to `var`, otherwise, propagates the error.
 *
 * @param var Variable to assign the value to on success.
 * @param ... Expression which returns `std::expected` type.
 *
 * @code
 * Identifier id {};
 * TRY_ASSIGN(id, identifier())
 * @endcode
 */
#define TRY_ASSIGN(var, ...)                                                 \
    {                                                                        \
        LBC_IGNORE_DIAGNOSTIC("-Wshadow", auto try_tmp_res = (__VA_ARGS__)); \
        if (try_tmp_res.has_value()) {                                       \
            (var) = std::move(try_tmp_res.value());                          \
        } else {                                                             \
            return std::unexpected(std::move(try_tmp_res.error()));          \
        }                                                                    \
    }

/**
 * Declares a variable and assigns it the value from an expression,
 * or return an error
 *
 * @param var Variable to declare and assign.
 * @param ... Expression which returns `std::expected` type.
 *
 * @code
 * TRY_DECL(var, expression())
 * @endcode
 */
#define TRY_DECL(var, ...)                                    \
    std::remove_cvref_t<decltype((__VA_ARGS__).value())> var; \
    TRY_ASSIGN(var, __VA_ARGS__);

/**
 * Executes an expression returning `std::expected`, on success
 * add the value to the sequence, otherwise, propagates the error.
 *
 * @param seq Sequencer to add the value to on success.
 * @param ... Expression which returns `std::expected` type.
 *
 * @code
 * Sequencer<AstVarDecl> seq {};
 * TRY_ADD(seq, varDecl());
 * @endcode
 */
#define TRY_ADD(seq, ...)                                                    \
    {                                                                        \
        LBC_IGNORE_DIAGNOSTIC("-Wshadow", auto try_tmp_res = (__VA_ARGS__)); \
        if (try_tmp_res) {                                                   \
            seq.add(std::move(try_tmp_res.value()));                         \
        } else {                                                             \
            return std::unexpected(std::move(try_tmp_res.error()));          \
        }                                                                    \
    }
