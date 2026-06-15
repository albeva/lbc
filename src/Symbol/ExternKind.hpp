//
// Created by Albert Varaksin on 15/06/2026.
//
#pragma once
#include "pch.hpp"
namespace lbc {

/**
 * Language linkage of a symbol. Determines how the symbol is named for ABI
 * purposes. Only "C" linkage is supported for now.
 */
enum class ExternKind : std::uint8_t {
    Default, ///< lbc's own linkage — canonical, upper-cased names
    C,       ///< C linkage — the symbol keeps its verbatim name for C ABI compatibility
};

} // namespace lbc
