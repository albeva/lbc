//
// Created by Albert on 19/02/2022.
//
#pragma once
#include <llvm/ADT/PointerIntPair.h>

namespace lbc {

/// Parse result wrapping pointer to type T
template<typename T>
class ParseResult;

/// Specialise for non-value ParseResult
template<>
class ParseResult<void> final {
public:
    ParseResult() noexcept = default;
    ParseResult(bool error) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_error{ error } {}

    [[nodiscard]] static ParseResult error() noexcept {
        return ParseResult{ true };
    }

    [[nodiscard]] inline bool isError() const noexcept {
        return m_error;
    }

private:
    bool m_error = false;
};

template<typename T>
class ParseResult final {
public:
    using value_type = T*;

    /// Default, null value, no error
    ParseResult() noexcept = default;

    /// Null value with defined error
    ParseResult(bool error) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ nullptr, error } {}

    /// given pointer value without error
    ParseResult(T* pointer) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ pointer, false } {}

    /// Downcast from derived type to base type
    template<typename U>
    requires std::is_base_of_v<T, U> ParseResult(ParseResult<U> other) // NOLINT(hicpp-explicit-conversions)
    noexcept
    : m_value{ other.getPointer(), other.isError() } {}

    /// cast from ParseResult<void>, null value and copy the error state
    ParseResult(ParseResult<void> other) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ nullptr, other.isError() } {}

    [[nodiscard]] static ParseResult error() noexcept {
        return ParseResult{ true };
    }

    [[nodiscard]] inline bool isError() const noexcept {
        return m_value.getInt();
    }

    [[nodiscard]] inline T* getPointer() const noexcept {
        return m_value.getPointer();
    }

private:
    const llvm::PointerIntPair<T*, 1, bool> m_value;
};

} // namespace lbc
