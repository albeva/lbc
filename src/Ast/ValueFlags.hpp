//
// Created by Albert on 06/06/2021.
//
#pragma once

namespace lbc {

struct ValueFlags final {
    enum class Kind : uint8_t {
        type,
        variable,
        function
    };
    Kind kind : 2;
    uint8_t addressable : 1;
    uint8_t dereferencable : 1;
    uint8_t assignable : 1;
    uint8_t external : 1;
    uint8_t unused1 : 1;
    uint8_t unused2 : 1;
};

} // namespace lbc
