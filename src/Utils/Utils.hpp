//
// Created by Albert Varaksin on 03/07/2020.
//
#pragma once
#include "pch.hpp"
// do not include pch.h!

#define LOG_VAR(VAR) llvm::outs() << #VAR << " = " << VAR << '\n';

#define NO_COPY_AND_MOVE(Class)         \
    Class(Class&&) = delete;            \
    Class(const Class&) = delete;       \
    Class& operator=(Class&&) = delete; \
    Class& operator=(const Class&) = delete;

#define CONCATENATE_DETAIL(x, y) x##y
#define CONCATENATE(x, y) CONCATENATE_DETAIL(x, y)
#define MAKE_UNIQUE(x) CONCATENATE(x, __COUNTER__)

namespace lbc {

// helper type for std::variant visitors
template<typename... Base>
struct Visitor : Base... {
    using Base::operator()...;
};

template<typename... Base>
Visitor(Base...) -> Visitor<Base...>;

// Helper to get the last type from a parameter pack
template<typename... Ts>
using LastType = typename decltype((std::type_identity<Ts>{}, ...))::type;

// Concept to test if type is a pointer
template<typename T>
concept IsPointer = std::is_pointer_v<T>;

// clang-format off
template<typename Derived, typename Base>
concept PointersDerivedFrom =
    std::is_pointer_v<Derived> &&
    std::is_pointer_v<Base> &&
    std::is_base_of_v<std::remove_pointer_t<Base>, std::remove_pointer_t<Derived>>;
// clang-format on

/**
 * Get Twine from "literal"_t
 */
inline llvm::Twine operator"" _t(const char* str, size_t /*len*/) {
    return { str };
}

/**
 * End compilation with the error message, clear the state and exit with error
 *
 * @param message to print
 * @param prefix add standard prefix before the message
 */
[[noreturn]] void fatalError(const llvm::Twine& message, bool prefix = true);

/**
 * Emit compiler warning, but continue compilation process
 *
 * @param message to print
 * @param prefix add standard prefix before the message
 */
void warning(const llvm::Twine& message, bool prefix = true);
} // namespace lbc

#include "Defer.hpp"
#include "Result.hpp"
#include "Try.hpp"
#include "ValueRestorer.hpp"
