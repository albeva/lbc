//
// Created by Albert on 19/02/2022.
//

#pragma once

// Idea is copied from Serenity OS
// https://github.com/SerenityOS/serenity/blob/master/AK/Try.h
//
// This breaks MSVC++ compatibility
//
// llvm::Expected<int> getValue();
// Expected<int> foo() {
//     int val = TRY(getValue());
//     useVal(val);
// }

#define TRY(expression) ({                   \
        auto resOrErr = (expression);        \
        if (auto err = resOrErr.takeError()) \
            return err;                      \
        *resOrErr;                           \
    })

#define TRY_FATAL(expression) ({                 \
        auto resOrErr = (expression);            \
        if (auto err = resOrErr.takeError())     \
            fatalError(                          \
                llvm::toString(std::move(err))); \
        *resOrErr;                               \
    })
