//
// Created by Albert on 19/02/2022.
//

#pragma once

namespace lbc {

// Idea is copied from Serenity OS
// https://github.com/SerenityOS/serenity/blob/master/AK/Try.h
//
// However, avoid GNU non-standard expression statement extension.
//
// Result<int> getValue();
// Result<int> foo() {
//     TRY_DECLARE(val, getValue());
//     useVal(val);
//     TRY_ASSIGN(val, getValue());
//     return val;
// }

#define TRY(expression)            \
    if ((expression).hasError()) { \
        return { ResultError{} };  \
    }

#define TRY_FATAL(expression)                         \
    if ((expression).hasError()) {                    \
        fatalError("TRY(" #expression ") has error"); \
    }

#define TRY_ASSIGN(var, expression)    \
    {                                  \
        auto valOrErr_ = (expression); \
        if (valOrErr_.hasError())      \
            return { ResultError{} };  \
        (var) = valOrErr_.getValue();  \
    }

#define TRY_DECLARE(var, expression)      \
    decltype(expression)::value_type var; \
    TRY_ASSIGN(var, expression);

} // namespace lbc
