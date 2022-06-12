//
// Created by Albert on 19/02/2022.
//
#pragma once
#include <llvm/ADT/PointerIntPair.h>

namespace lbc {

// Tag type to indicate error
struct ResultError final {};

/// Parse result wrapping pointer to type T
template<typename T = void>
class Result;

/// Specialise for no type
template<std::same_as<void> T>
class [[nodiscard]] Result<T> final {
public:
    constexpr Result() noexcept = default;

    constexpr Result(ResultError /* _ */) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_hasError{ true } {}

    [[nodiscard]] constexpr static Result makeError() noexcept {
        return Result{ ResultError{} };
    }

    [[nodiscard]] constexpr inline bool hasError() const noexcept {
        return m_hasError;
    }

    [[nodiscard]] constexpr inline ResultError getError() const noexcept {
        assert(hasError() && "getting error from non erronous Result");
        return ResultError{};
    }

private:
    const bool m_hasError = false;
};

// Specialize for pointers
template<IsPointer T>
class [[nodiscard]] Result<T> final {
public:
    using value_type = T;

    /// Default, null value, no error
    constexpr Result() noexcept = default;

    /// Null value with defined error
    constexpr Result(ResultError /* _ */) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ nullptr, true } {}

    /// given pointer value without error
    constexpr Result(T pointer) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ pointer, false } {}

    /// Downcast from derived type to base type
    template<PointersDerivedFrom<T> U>
    constexpr Result(const Result<U>& other) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ other.getValue(), other.hasError() } {}

    /// cast from ParseResult<>, null value and copy the error state
    constexpr Result(const Result<void>& other) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ nullptr, other.hasError() } {}

    [[nodiscard]] constexpr static Result makeError() noexcept {
        return Result{ ResultError{} };
    }

    [[nodiscard]] constexpr inline bool hasError() const noexcept {
        return m_value.getInt();
    }

    [[nodiscard]] constexpr inline ResultError getError() const noexcept {
        assert(hasError() && "getting error from non erronous Result");
        return ResultError{};
    }

    [[nodiscard]] constexpr inline T getValue() const noexcept {
        return m_value.getPointer();
    }

private:
    const llvm::PointerIntPair<T, 1, bool> m_value;
};

// Specialise for any non pointer which is copyable
template<std::movable T>
    requires(not IsPointer<T>)
class [[nodiscard]] Result<T> final {
public:
    using value_type = T;

    // Default value
    constexpr Result() noexcept
        requires std::is_default_constructible_v<T>
    : m_value{ std::in_place_type<T> } {}

    constexpr Result(const Result&) noexcept requires std::copyable<T> = default;
    constexpr Result(Result&&) noexcept = default;
    ~Result() noexcept = default;

    Result& operator= (const Result&) = delete;
    Result& operator= (Result&&) = delete;

    // Error
    constexpr Result(ResultError /* _ */) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ std::in_place_type<std::monostate> } {};

    // Move value
    constexpr Result(T&& value) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ std::forward<T>(value) } {}

    // Copy value
    constexpr Result(const T& value) noexcept // NOLINT(hicpp-explicit-conversions)
        requires std::copyable<T>
    : m_value{ value } {}

    /// From Result<void> only if value is default constructible
    constexpr Result(const Result<void>& other) noexcept // NOLINT(hicpp-explicit-conversions)
        requires std::is_default_constructible_v<T>
    : m_value{ std::in_place_type<std::monostate> }
    {
        if (!other.hasError()) {
            m_value.template emplace<T>();
        }
    }

    /// Downcast from derived type to base type
    template<std::convertible_to<T> U>
    constexpr Result(Result<U>&& other) noexcept // NOLINT(hicpp-explicit-conversions)
        requires std::movable<U>
    : m_value{ std::in_place_type<std::monostate> }
    {
        if (!other.hasError()) {
            m_value = static_cast<T&&>(std::move(other.getValue()));
        }
    }

    template<std::convertible_to<T> U>
    constexpr Result(const Result<U>& other) noexcept // NOLINT(hicpp-explicit-conversions)
        requires std::copyable<U>
    : m_value{ std::in_place_type<std::monostate> }
    {
        if (!other.hasError()) {
            m_value = static_cast<T>(other.getValue());
        }
    }

    [[nodiscard]] constexpr static Result makeError() noexcept {
        return Result{ ResultError{} };
    }

    [[nodiscard]] constexpr inline bool hasError() const noexcept {
        return std::holds_alternative<std::monostate>(m_value);
    }

    [[nodiscard]] constexpr inline ResultError getError() const noexcept {
        assert(hasError() && "getting error from non erronous Result");
        return ResultError{};
    }

    [[nodiscard]] constexpr inline T& getValue() & noexcept {
        return std::get<T>(m_value);
    }

    [[nodiscard]] constexpr inline const T& getValue() const& noexcept {
        return std::get<T>(m_value);
    }

    [[nodiscard]] constexpr inline T&& getValue() && noexcept {
        return std::move(std::get<T>(m_value));
    }

    [[nodiscard]] constexpr inline const T&& getValue() const&& noexcept {
        return std::move(std::get<T>(m_value));
    }

private:
    std::variant<std::monostate, T> m_value;
};

} // namespace lbc
