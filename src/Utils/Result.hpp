//
// Created by Albert on 19/02/2022.
//
#pragma once
#include <llvm/ADT/PointerIntPair.h>

namespace lbc {

enum class ResultStatus : bool {
    valid,
    error
};

/// Parse result wrapping pointer to type T
template<typename T>
class Result;

/// Specialise for non-value ParseResult
template<>
class [[nodiscard]] Result<void> final {
public:
    constexpr Result() noexcept = default;
    constexpr Result(ResultStatus error) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_error{ error } {}

    [[nodiscard]] constexpr static Result error() noexcept {
        return Result{ ResultStatus::error };
    }

    [[nodiscard]] constexpr inline bool isError() const noexcept {
        return m_error == ResultStatus::error;
    }

private:
    const ResultStatus m_error = ResultStatus::valid;
};

template<typename T>
class [[nodiscard]] Result final {
public:
    using value_type = T*;

    /// Default, null value, no error
    constexpr Result() noexcept = default;

    /// Null value with defined error
    constexpr Result(ResultStatus error) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ nullptr, error } {}

    /// given pointer value without error
    constexpr Result(T* pointer) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ pointer, ResultStatus::valid } {}

    /// Downcast from derived type to base type
    template<std::derived_from<T> U>
    constexpr Result(const Result<U>& other) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ other.getPointer(), other.isError() ? ResultStatus::error : ResultStatus::valid } {}

    /// cast from ParseResult<void>, null value and copy the error state
    constexpr Result(const Result<void>& other) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ nullptr, other.isError() ? ResultStatus::error : ResultStatus::valid } {}

    [[nodiscard]] constexpr static Result error() noexcept {
        return Result{ ResultStatus::error };
    }

    [[nodiscard]] constexpr inline bool isError() const noexcept {
        return m_value.getInt() == ResultStatus::error;
    }

    [[nodiscard]] constexpr inline T* getPointer() const noexcept {
        return m_value.getPointer();
    }

private:
    const llvm::PointerIntPair<T*, 1, ResultStatus> m_value;
};

} // namespace lbc
