//
// Created by Albert on 19/02/2022.
//
#pragma once
#include "pch.hpp"

namespace lbc {

// Tag type to indicate error
struct ResultError final {};

/// Parse result wrapping pointer to type T
template<typename T>
class Result;

/// Specialise for no type
template<>
class [[nodiscard]] Result<void> final {
public:
    using value_type = void;

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
    using base_type = std::remove_pointer_t<T>;

    constexpr Result() = default;

    /// Null value with defined error
    // cppcheck-suppress noExplicitConstructor
    constexpr Result(ResultError /* _ */) // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
    : m_hasError{ true } {}

    constexpr Result& operator=(ResultError /* _ */) {
        m_hasError = true;
        m_value = nullptr;
        return *this;
    }

    /// given pointer value without error
    // cppcheck-suppress noExplicitConstructor
    constexpr Result(T pointer) // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
    : m_value{ pointer } {}

    constexpr Result& operator=(T pointer) {
        m_hasError = false;
        m_value = pointer;
        return *this;
    }

    /// Downcast from derived type to base type
    template<typename U>
    friend class Result;

    template<PointersDerivedFrom<T> U>
    constexpr Result(const Result<U>& other) // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
    : m_hasError{ other.m_hasError }, m_value{ other.m_value } {}

    template<PointersDerivedFrom<T> U>
    constexpr inline Result& operator=(const Result<U>& other) {
        m_hasError = other.m_hasError;
        m_value = other.m_value;
        return *this;
    }

    [[nodiscard]] constexpr inline bool hasError() const {
        return m_hasError;
    }

    [[nodiscard]] constexpr inline T getValue() const {
        assert(not m_hasError && "Getting value from erronous Result");
        return m_value;
    }

    [[nodiscard]] constexpr inline T getValueOrNull() const {
        if (m_hasError) {
            return nullptr;
        }
        return m_value;
    }

    [[nodiscard]] constexpr inline T operator->() const {
        return getValue();
    }

    [[nodiscard]] constexpr inline std::add_lvalue_reference_t<base_type> operator*() const {
        return *getValue();
    }

    bool constexpr inline operator==(std::nullptr_t) const {
        return getValueOrNull() == nullptr;
    }

    bool constexpr inline operator!=(std::nullptr_t) const {
        return getValueOrNull() != nullptr;
    }

    /// Since Result value is not required and can be null, casting to bool is ambigious
    ///
    /// Use following to check:
    ///    auto result = getResult();
    ///    if (result != nullptr) { ... }                     // if result contains a valid pointer
    ///    if (auto* value = result.getValueOrNull()) { ... } // similar to above, but also bind the value
    ///    if (!result.hasError()) { ... }                    // check that result does not contain an error
    operator bool() = delete;
    operator bool() const = delete;

    /// Implicitly cast result to type T. This is UNSAFE and must be checked for error first!
    /* explicit */ constexpr inline operator T () const { // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
        return getValue();
    }

private:
    T m_value{};
    bool m_hasError{};
};

} // namespace lbc
