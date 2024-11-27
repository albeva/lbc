//
// Created by Albert on 22/02/2022.
//
#pragma once
#include "pch.hpp"

namespace lbc::detail {
struct DeferTask { };
template <typename F>
auto operator+(DeferTask /* task */, F&& fn) -> decltype(llvm::make_scope_exit(std::forward<F>(fn))) {
    return llvm::make_scope_exit(std::forward<F>(fn));
}
} // namespace lbc::detail

/// Execute code when exiting current scope, similar to Swift "defer" statements.
/// DEFER {  do_work_on_exit(); };
#define DEFER auto MAKE_UNIQUE(deferFunc) = ::lbc::detail::DeferTask() + [&]()
