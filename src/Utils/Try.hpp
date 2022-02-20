//
// Created by Albert on 19/02/2022.
//

#pragma once

// Idea is copied from Serenity OS
// https://github.com/SerenityOS/serenity/blob/master/AK/Try.h
//
// llvm::Expected<int> getValue();
// Expected<int> foo() {
//     TRY_DECLARE(val, getValue());
//     useVal(val);
//     TRY_ASSIGN(val, getValue());
// }

#define TRY(expression)                      \
    if (auto err = (expression).takeError()) \
        return err;

#define TRY_ASSIGN(var, expression)                    \
    {                                                  \
        auto tryAssignValOrErr_ = (expression);        \
        if (auto err = tryAssignValOrErr_.takeError()) \
            return err;                                \
        var = *tryAssignValOrErr_;                     \
    }

#define TRY_DECLARE(var, expression)      \
    decltype(expression)::value_type var; \
    TRY_ASSIGN(var, expression);
