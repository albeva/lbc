//
// Created by Albert Varaksin on 11/02/2026.
//
#pragma once

// STL
#include <array>
#include <cassert>
#include <cstdint>
#include <expected>
#include <format>
#include <print>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

// LLVM
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/SMLoc.h>
#include <llvm/Support/SourceMgr.h>

// LBC utilities
#include "Utilities/NoCopy.hpp"

namespace lbc {
using namespace std::string_view_literals;
using namespace std::string_literals;
} // namespace lbc
