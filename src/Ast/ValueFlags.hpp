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
    uint8_t assignable : 1;
    uint8_t external : 1;
};

} // namespace lbc
