//
// Created by Albert on 19/02/2022.
//

#pragma once

namespace lbc {

/**
 * @def TRY(expression)
 * @brief A macro that checks if the given expression has an error. If it does, it returns a ResultError.
 * @param expression The expression to check for errors. This should be a call to a function that returns a Result.
 */
#define TRY(expression)            \
    if ((expression).hasError()) { \
        return ResultError {};     \
    }

/**
 * @def MUST(expression)
 * @brief A macro that checks if the given expression has an error. If it does, it triggers a fatal error.
 * @param expression The expression to check for errors. This should be a call to a function that returns a Result.
 */
#define MUST(expression)                                              \
    if ((expression).hasError()) {                                    \
        fatalError("MUST(" #expression ") has error. ", false, true); \
    }

/**
 * @def TRY_ASSIGN(var, expression)
 * @brief A macro that checks if the given expression has an error. If it does, it returns a ResultError.
 *        Otherwise, it assigns the result of the expression to the provided variable.
 * @param var The variable to which the result of the expression should be assigned.
 * @param expression The expression to check for errors. This should be a call to a function that returns a Result.
 */
#define TRY_ASSIGN(var, expression) \
    {                               \
        auto result = expression;   \
        TRY(result)                 \
        var = result.getValue();    \
    }

/**
 * @def TRY_DECL(var, expression)
 * @brief A macro that declares a variable of the type returned by the expression, checks if the given expression has an error,
 *        and if it does not, assigns the result of the expression to the newly declared variable.
 * @param var The name of the variable to be declared.
 * @param expression The expression to check for errors and whose return type is used for the variable declaration. This should be a call to a function that returns a Result.
 */
#define TRY_DECL(var, expression)          \
    decltype((expression).getValue()) var; \
    TRY_ASSIGN(var, expression)

} // namespace lbc
