//
// Created by Albert Varaksin on 03/07/2020.
//
#pragma once
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
struct Visitor : Base... { using Base::operator()...; };

template<typename... Base>
Visitor(Base...) -> Visitor<Base...>;

/**
 * Get Twine from "literal"_t
 */
inline llvm::Twine operator"" _t(const char* str, size_t /*len*/) noexcept {
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
 * @param message to print
 * @param prefix add standard prefix before the message
 */
void warning(const llvm::Twine& message, bool prefix = true);
} // namespace lbc

#include "ScopeGuard.hpp"
#include "ValueRestorer.hpp"
