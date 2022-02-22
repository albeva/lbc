//
// Created by Albert on 19/02/2022.
//

#pragma once

namespace lbc {

// Idea is copied from Serenity OS
// https://github.com/SerenityOS/serenity/blob/master/AK/Try.h
//
// llvm::Expected<int> getValue();
// Expected<int> foo() {
//     TRY_DECLARE(val, getValue());
//     useVal(val);
//     TRY_ASSIGN(val, getValue());
// }

template<typename T>
inline llvm::Error try_resolve_to_error(llvm::Expected<T> expected) noexcept {
    return expected.takeError();
}

inline llvm::Error try_resolve_to_error(llvm::Error error) noexcept {
    return error;
}

#define TRY(expression)                                \
    if (auto err = try_resolve_to_error(expression)) { \
        return std::forward<llvm::Error>(err);         \
    }

#define TRY_ASSIGN(var, expression)                    \
    {                                                  \
        auto tryAssignValOrErr_ = (expression);        \
        if (auto err = tryAssignValOrErr_.takeError()) \
            return std::move(err);                     \
        (var) = *tryAssignValOrErr_;                   \
    }

#define TRY_DECLARE(var, expression)      \
    decltype(expression)::value_type var; \
    TRY_ASSIGN(var, expression);

} // namespace lbc
