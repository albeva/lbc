//
// Created by Albert on 06/06/2021.
//
#pragma once
#include "pch.hpp"

namespace lbc {

struct ValueFlags final {
    enum class Kind : uint8_t {
        Type,
        Variable,
        Function
    };
    // Value kind
    Kind kind : 2;
    /// Can assign value
    uint8_t assignable : 1;
    /// Can take address
    uint8_t addressable : 1;
    /// Is a constant
    uint8_t constant : 1;
};
static_assert(sizeof(ValueFlags) == 1);

} // namespace lbc
