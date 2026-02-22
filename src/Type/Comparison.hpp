//
// Created by Albert Varaksin on 22/02/2026.
//
#pragma once
#include "pch.hpp"
namespace lbc {

struct TypeComparisonResult final {
    enum Result : std::uint8_t { // NOLINT(*-use-enum-class)
        Incompatible,
        Convertible,
        Identical
    };

    enum class Flags : std::uint8_t {
        Unchanged = 0,
        Added = 1U << 0U,
        Removed = 1U << 1U
    };

    Result result   : 4 = Result::Incompatible;
    Flags sign      : 2 = Flags::Unchanged;
    Flags reference : 2 = Flags::Unchanged;
    Flags size      : 2 = Flags::Unchanged;
    Flags precision : 2 = Flags::Unchanged;

    TypeComparisonResult(const Result res) // NOLINT(*-explicit-conversions)
    : result(res) { }
};
MARK_AS_FLAGS_ENUM(TypeComparisonResult::Flags);

} // namespace lbc
