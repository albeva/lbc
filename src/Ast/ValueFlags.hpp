//
// Created by Albert on 06/06/2021.
//
#pragma once

namespace lbc {

struct ValueFlags final {
    uint8_t addressable : 1;
    uint8_t dereferencable : 1;
    uint8_t assignable : 1;
    uint8_t callable : 1;
    uint8_t isType : 1;
    uint8_t isExternal : 1;
};

} // namespace lbc
