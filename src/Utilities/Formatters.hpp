//
// Created by Albert on 17/02/2026.
//
#pragma once
#include "pch.hpp"

/**
 * std::format and std::print support for std::source_location
 */
template <>
struct std::formatter<std::source_location, char> final {
    static constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const std::source_location& loc, std::format_context& ctx) {
        return std::format_to(
            ctx.out(),
            "{}:{}:{} ({})",
            loc.file_name(),
            loc.line(),
            loc.column(),
            loc.function_name()
        );
    }
};
