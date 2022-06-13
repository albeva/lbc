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

private:
    bool m_hasError = false;
};

// Specialize for pointers
template<IsPointer T>
class [[nodiscard]] Result<T> final {
public:
    using value_type = T;

    // default to null, valid
    constexpr Result() noexcept : m_value{ nullptr, false } {}

    /// Null value with defined error
    constexpr Result(ResultError /* _ */) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ nullptr, true } {}

    /// given pointer value without error
    constexpr Result(T pointer) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ pointer, false } {}

    constexpr Result& operator=(T pointer) noexcept {
        m_value.setPointerAndInt(pointer, false);
        return *this;
    }

    constexpr Result& operator=(ResultError /* _ */) noexcept {
        m_value.setPointerAndInt(nullptr, true);
        return *this;
    }

    /// Downcast from derived type to base type
    template<PointersDerivedFrom<T> U>
    constexpr Result(Result<U>&& other) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ other.hasError() ? nullptr : other.getValue(), other.hasError() } {}

    template<PointersDerivedFrom<T> U>
    constexpr Result& operator=(Result<U>&& other) noexcept {
        if (other.hasError()) {
            m_value.setPointerAndInt(nullptr, true);
        } else {
            m_value.setPointerAndInt(other.getValue(), false);
        }
        return *this;
    }

    /// cast from ParseResult<>, null value and copy the error state
    constexpr Result(Result<void>&& other) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ nullptr, other.hasError() } {}

    [[nodiscard]] constexpr static Result makeError() noexcept {
        return Result{ ResultError{} };
    }

    [[nodiscard]] constexpr inline bool hasError() const noexcept {
        return m_value.getInt();
    }

    [[nodiscard]] constexpr inline T getValue() const noexcept {
        assert(not hasError() && "Getting value from erronous Result");
        return m_value.getPointer();
    }

    [[nodiscard]] constexpr inline T getValueOrNull() const noexcept {
        if (hasError()) {
            return nullptr;
        }
        return m_value.getPointer();
    }

    [[nodiscard]] constexpr inline T operator->() const noexcept {
        return getValue();
    }

    [[nodiscard]] constexpr inline T operator*() const noexcept {
        return getValue();
    }

private:
    llvm::PointerIntPair<T, 1, bool> m_value;
};

// Specialise for any non pointer which is copyable
template<std::copyable T>
    requires(not IsPointer<T>)
class [[nodiscard]] Result<T> final {
public:
    using value_type = T;

    // Default value
    constexpr Result() noexcept
        requires std::is_default_constructible_v<T>
    : m_value{ std::in_place_type<T> } {}

    // Error
    constexpr Result(ResultError /* _ */) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{} {};

    // Move value
    constexpr Result(T&& value) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ std::forward<T>(value) } {}

    // Copy value
    constexpr Result(const T& value) noexcept // NOLINT(hicpp-explicit-conversions)
    : m_value{ value } {}

    /// From Result<void> only if value is default constructible
    constexpr Result(const Result<void>& other) noexcept // NOLINT(hicpp-explicit-conversions)
        requires std::is_default_constructible_v<T>
    : m_value{} {
        if (!other.hasError()) {
            m_value.template emplace<T>();
        }
    }

    /// Downcast from derived type to base type
    template<std::convertible_to<T> U>
    constexpr Result(Result<U>&& other) noexcept // NOLINT(hicpp-explicit-conversions)
        requires std::movable<U>
    : m_value{} {
        if (!other.hasError()) {
            m_value = static_cast<T&&>(std::move(other.getValue()));
        }
    }

    template<std::convertible_to<T> U>
    constexpr Result(const Result<U>& other) noexcept // NOLINT(hicpp-explicit-conversions)
        requires std::copyable<U>
    : m_value{} {
        if (!other.hasError()) {
            m_value = static_cast<T>(other.getValue());
        }
    }

    [[nodiscard]] constexpr static Result makeError() noexcept {
        return Result{ ResultError{} };
    }

    [[nodiscard]] constexpr inline bool hasError() const noexcept {
        return not m_value.has_value();
    }

    [[nodiscard]] constexpr inline T getValueOrDefault() const noexcept
        requires std::is_default_constructible_v<T> && std::copyable<T>
    {
        if (hasError()) {
            return T();
        }
        return m_value.getPointer();
    }

    [[nodiscard]] constexpr inline T& getValue() & noexcept {
        assert(not hasError() && "Getting value from erronous Result");
        return m_value.value();
    }

    [[nodiscard]] constexpr inline const T& getValue() const& noexcept {
        assert(not hasError() && "Getting value from erronous Result");
        return m_value.value();
    }

    [[nodiscard]] constexpr inline T&& getValue() && noexcept {
        assert(not hasError() && "Getting value from erronous Result");
        return std::move(m_value.value());
    }

    [[nodiscard]] constexpr inline const T&& getValue() const&& noexcept {
        assert(not hasError() && "Getting value from erronous Result");
        return std::move(m_value.value());
    }

    [[nodiscard]] constexpr inline const T* operator->() const noexcept {
        return &getValue();
    }

    [[nodiscard]] constexpr inline T* operator->() noexcept {
        return &getValue();
    }

    [[nodiscard]] constexpr const T& operator*() const& noexcept {
        return getValue();
    }

    [[nodiscard]] constexpr T& operator*() & noexcept {
        return getValue();
    }

    [[nodiscard]] constexpr const T&& operator*() const&& noexcept {
        return getValue();
    }

    [[nodiscard]] constexpr T&& operator*() && noexcept {
        return getValue();
    }

private:
    std::optional<T> m_value;
};

} // namespace lbc
