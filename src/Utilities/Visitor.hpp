#pragma once
#include "pch.hpp"
namespace lbc {

template <typename... Base>
struct Visitor : Base... {
    using Base::operator()...;
};

} // namespace lbc
