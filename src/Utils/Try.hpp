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
// ParseError<int> getValue();
// ParseError<int> foo() {
//     TRY_DECLARE(val, getValue());
//     useVal(val);
//     TRY_ASSIGN(val, getValue());
//     return val;
// }

#define TRY(expression)                 \
    if ((expression).isError()) {       \
        return { ResultStatus::error }; \
    }

#define TRY_ASSIGN(var, expression)         \
    {                                       \
        auto valOrErr_ = (expression);      \
        if (valOrErr_.isError())            \
            return { ResultStatus::error }; \
        (var) = valOrErr_.getPointer();     \
    }

#define TRY_DECLARE(var, expression)      \
    decltype(expression)::value_type var; \
    TRY_ASSIGN(var, expression);

} // namespace lbc
