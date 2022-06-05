//
// Created by Albert on 19/02/2022.
//
#pragma once
#include <llvm/ADT/PointerIntPair.h>

namespace lbc {

enum class ParseStatus : bool {
    valid,
    error
};

/// Parse result wrapping pointer to type T
template<typename T>
class ParseResult;

/// Specialise for non-value ParseResult
template<>
class [[nodiscard]] ParseResult<void> final {
public:
    constexpr ParseResult() noexcept = default;
    constexpr ParseResult(ParseStatus error) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_error{ error } {}

    [[nodiscard]] constexpr static ParseResult error() noexcept {
        return ParseResult{ ParseStatus::error };
    }

    [[nodiscard]] constexpr inline bool isError() const noexcept {
        return m_error == ParseStatus::error;
    }

private:
    const ParseStatus m_error = ParseStatus::valid;
};

template<typename T>
class [[nodiscard]] ParseResult final {
public:
    using value_type = T*;

    /// Default, null value, no error
    constexpr ParseResult() noexcept = default;

    /// Null value with defined error
    constexpr ParseResult(ParseStatus error) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ nullptr, error } {}

    /// given pointer value without error
    constexpr ParseResult(T* pointer) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ pointer, ParseStatus::valid } {}

    /// Downcast from derived type to base type
    template<std::derived_from<T> U>
    constexpr ParseResult(ParseResult<U> other) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ other.getPointer(), other.isError() ? ParseStatus::error : ParseStatus::valid } {}

    /// cast from ParseResult<void>, null value and copy the error state
    constexpr ParseResult(ParseResult<void> other) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ nullptr, other.isError() ? ParseStatus::error : ParseStatus::valid } {}

    [[nodiscard]] constexpr static ParseResult error() noexcept {
        return ParseResult{ ParseStatus::error };
    }

    [[nodiscard]] constexpr inline bool isError() const noexcept {
        return m_value.getInt() == ParseStatus::error;
    }

    [[nodiscard]] constexpr inline T* getPointer() const noexcept {
        return m_value.getPointer();
    }

private:
    const llvm::PointerIntPair<T*, 1, ParseStatus> m_value;
};

} // namespace lbc
