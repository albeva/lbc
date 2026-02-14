//
// Created by Albert Varaksin on 13/02/2026.
//
#pragma once
#include "pch.hpp"

namespace lbc {

struct LiteralValue final : std::variant<std::monostate, bool, std::uint64_t, double, std::string_view> {
    using variant::variant;
};

} // namespace lbc
