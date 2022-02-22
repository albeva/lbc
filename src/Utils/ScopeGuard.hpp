//
// Created by Albert on 22/02/2022.
//
#pragma once
namespace lbc {

template<typename T>
struct ScopeGuard final {
    NO_COPY_AND_MOVE(ScopeGuard)
    constexpr explicit ScopeGuard(T&& handler) : handler{ std::forward<T>(handler) } {}

    ~ScopeGuard() {
        handler();
    }
private:
    const T handler;
};
template<typename F> ScopeGuard(F&&) -> ScopeGuard<F>;

/**
 * Execute given statement when existing the scope
 *
 * SCOPE_GAURD(closeFile(file))
 */
#define SCOPE_GAURD(HANDLER) \
    ScopeGuard MAKE_UNIQUE(tmp_scope_giard_) { [&]() { HANDLER; } } /* NOLINT */

} // namespace lbc
