//
// Created by Albert on 22/02/2022.
//
#pragma once
#include "pch.hpp"
namespace lbc {

template<typename T>
    requires std::is_trivial_v<T>
struct ValueRestorer final {
    NO_COPY_AND_MOVE(ValueRestorer)
    constexpr explicit ValueRestorer(T& value) noexcept : m_target{ value }, m_value{ value } {}

    ~ValueRestorer() noexcept {
        m_target = m_value;
    }

private:
    T& m_target;
    T const m_value;
};

/**
 * Helper macro that restores variable value when existing scope.
 *
 * RESTORE_ON_EXIT(m_symbolTable)
 */
#define RESTORE_ON_EXIT(V) \
    ValueRestorer<decltype(V)> MAKE_UNIQUE(tmp_restore_onexit_) { V } /* NOLINT */

} // namespace lbc
