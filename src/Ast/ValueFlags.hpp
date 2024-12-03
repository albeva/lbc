//
// Created by Albert on 06/06/2021.
//
#pragma once
#include "pch.hpp"

namespace lbc {

struct ValueFlags final {
    enum class Kind : uint8_t {
        type,
        variable,
        function
    };
    Kind kind : 2;
    /// Symbol can be assigned a value if:
    /// - is a variable
    /// - variable is not a constant or readonly
    uint8_t assignable : 1;
    uint8_t external : 1;
    uint8_t mustBeConstant : 1;
    uint8_t reference : 1;
    /// Can take address of the expression. E.g. sub, function or a variable
    uint8_t addressable : 1;
};

} // namespace lbc
