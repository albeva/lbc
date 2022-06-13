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
//   auto res = getValue();
//   TRY(res)
//   std::cout << "value = " << *res << '\n';
// }

#define TRY(expression)            \
    if ((expression).hasError()) { \
        return ResultError{};      \
    }

#define TRY_FATAL(expression)                         \
    if ((expression).hasError()) {                    \
        fatalError("TRY(" #expression ") has error"); \
    }

} // namespace lbc
