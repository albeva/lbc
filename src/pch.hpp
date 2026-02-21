//
// Created by Albert Varaksin on 11/02/2026.
//
#pragma once

// STL
#include <array>
#include <cassert>
#include <charconv>
#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <print>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

// LLVM
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/Support/Allocator.h>
#include <llvm/Support/SMLoc.h>
#include <llvm/Support/SourceMgr.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

// LBC utilities
#include "Utilities/BaseFlags.hpp"
#include "Utilities/Formatters.hpp"
#include "Utilities/NoCopy.hpp"
#include "Utilities/Try.hpp"
#include "Utilities/ValueRestorer.hpp"
#include "Utilities/Visitor.hpp"

namespace lbc {
using namespace std::string_view_literals;
using namespace std::string_literals;
} // namespace lbc
