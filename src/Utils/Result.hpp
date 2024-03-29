//
// Created by Albert on 19/02/2022.
//
#pragma once
#include "pch.hpp"
#include <llvm/ADT/PointerIntPair.h>

namespace lbc {

// Tag type to indicate error
struct ResultError final {};

/// Parse result wrapping pointer to type T
template<typename T>
class Result;

/// Specialise for no type
template<std::same_as<void> T>
class [[nodiscard]] Result<T> final {
public:
    using value_type = T;

    constexpr Result() = default;

    // cppcheck-suppress noExplicitConstructor
    /* explicit */ constexpr Result(ResultError /* _ */) // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
    : m_hasError{ true } {}

    constexpr Result& operator=(ResultError /* _ */) {
        m_hasError = true;
        return *this;
    }

    [[nodiscard]] constexpr inline bool hasError() const {
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
    constexpr Result() : m_value{ nullptr, false } {}

    /// Null value with defined error
    // cppcheck-suppress noExplicitConstructor
    constexpr Result(ResultError /* _ */) // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
    : m_value{ nullptr, true } {}

    constexpr Result& operator=(ResultError /* _ */) {
        m_value.setPointerAndInt(nullptr, true);
        return *this;
    }

    /// given pointer value without error
    // cppcheck-suppress noExplicitConstructor
    constexpr Result(T pointer) // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
    : m_value{ pointer, false } {}

    constexpr Result& operator=(T pointer) {
        m_value.setPointerAndInt(pointer, false);
        return *this;
    }

    /// Downcast from derived type to base type
    template<typename U>
    friend class Result;

    template<PointersDerivedFrom<T> U>
    constexpr Result(Result<U>&& other) // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
    : m_value{ other.m_value.getPointer(), other.m_value.getInt() } {}

    template<PointersDerivedFrom<T> U>
    constexpr inline Result& operator=(Result<U>&& other) {
        m_value.setPointerAndInt(other.m_value.getPointer(), other.m_value.getInt());
        return *this;
    }

    [[nodiscard]] constexpr inline bool hasError() const {
        return m_value.getInt();
    }

    [[nodiscard]] constexpr inline T getValue() const {
        assert(not hasError() && "Getting value from erronous Result");
        return m_value.getPointer();
    }

    [[nodiscard]] constexpr inline T getValueOrNull() const {
        if (hasError()) {
            return nullptr;
        }
        return m_value.getPointer();
    }

    [[nodiscard]] constexpr inline T operator->() const {
        return getValue();
    }

    [[nodiscard]] constexpr inline T operator*() const {
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
    constexpr Result()
        requires std::is_default_constructible_v<T>
    : m_value{ std::in_place_type<T> } {}

    // Error
    // cppcheck-suppress noExplicitConstructor
    constexpr Result(ResultError /* _ */) // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
    : m_value{} {}

    constexpr inline Result& operator=(ResultError /* _ */) {
        m_value.reset();
        return *this;
    }

    // Move value
    // cppcheck-suppress noExplicitConstructor
    constexpr Result(T&& value) // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
    : m_value{ std::forward<T>(value) } {}

    constexpr inline Result& operator=(T&& other) {
        m_value = std::move(other.m_value);
        return *this;
    }

    // Copy value
    // cppcheck-suppress noExplicitConstructor
    constexpr Result(const T& value) // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
    : m_value{ value } {}

    constexpr inline Result& operator=(const T& other) {
        m_value = other.m_value;
        return *this;
    }

    /// Downcast from derived type to base type
    // cppcheck-suppress noExplicitConstructor
    template<std::convertible_to<T> U>
    constexpr Result(Result<U>&& other) // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
        requires std::movable<U>
    : m_value{} {
        if (!other.hasError()) {
            m_value = static_cast<T&&>(std::move(other.getValue()));
        }
    }

    template<std::convertible_to<T> U>
    constexpr inline Result& operator=(Result<U>&& other)
        requires std::movable<U>
    {
        if (other.hasError()) {
            m_value.reset();
        } else {
            m_value = static_cast<T&&>(std::move(other.getValue()));
        }
    }

    // cppcheck-suppress noExplicitConstructor
    template<std::convertible_to<T> U>
    constexpr Result(const Result<U>& other) // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
        requires std::copyable<U>
    : m_value{} {
        if (!other.hasError()) {
            m_value = static_cast<T>(other.getValue());
        }
    }

    template<std::convertible_to<T> U>
    constexpr inline Result& operator=(const Result<U>& other)
        requires std::copyable<U>
    {
        if (other.hasError()) {
            m_value.reset();
        } else {
            m_value = static_cast<T>(other.getValue());
        }
    }

    [[nodiscard]] constexpr inline bool hasError() const {
        return not m_value.has_value();
    }

    [[nodiscard]] constexpr inline T getValueOrDefault() const
        requires std::is_default_constructible_v<T> && std::copyable<T>
    {
        if (hasError()) {
            return T();
        }
        return m_value.value();
    }

    [[nodiscard]] constexpr inline T& getValue() & {
        assert(not hasError() && "Getting value from erronous Result");
        return m_value.value();
    }

    [[nodiscard]] constexpr inline const T& getValue() const& {
        assert(not hasError() && "Getting value from erronous Result");
        return m_value.value();
    }

    [[nodiscard]] constexpr inline T&& getValue() && {
        assert(not hasError() && "Getting value from erronous Result");
        return std::move(m_value.value());
    }

    [[nodiscard]] constexpr inline const T&& getValue() const&& {
        assert(not hasError() && "Getting value from erronous Result");
        return std::move(m_value.value());
    }

    [[nodiscard]] constexpr inline const T* operator->() const {
        return &getValue();
    }

    [[nodiscard]] constexpr inline T* operator->() {
        return &getValue();
    }

    [[nodiscard]] constexpr const T& operator*() const& {
        return getValue();
    }

    [[nodiscard]] constexpr T& operator*() & {
        return getValue();
    }

    [[nodiscard]] constexpr const T&& operator*() const&& {
        return getValue();
    }

    [[nodiscard]] constexpr T&& operator*() && {
        return getValue();
    }

private:
    std::optional<T> m_value;
};

} // namespace lbc
