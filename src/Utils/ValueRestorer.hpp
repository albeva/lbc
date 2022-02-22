//
// Created by Albert on 22/02/2022.
//
#pragma once
namespace lbc {

template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T> && std::is_trivially_assignable_v<T&, T>, int> = 0>
struct ValueRestorer final {
    NO_COPY_AND_MOVE(ValueRestorer)
    constexpr explicit ValueRestorer(T& value) noexcept : m_target{ value }, m_value{ value } {}

    ~ValueRestorer() noexcept {
        m_target = m_value;
    }

private:
    T& m_target;
    const T m_value;
};

/**
 * Helper macro that restores variable value when existing scope.
 *
 * RESTORE_ON_EXIT(m_symbolTable)
 */
#define RESTORE_ON_EXIT(V) \
    ValueRestorer<decltype(V)> MAKE_UNIQUE(tmp_restore_onexit_) { V } /* NOLINT */

} // namespace lbc
