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

    constexpr Result() = default;

    // cppcheck-suppress noExplicitConstructor
    /* explicit */ constexpr Result(ResultError /* _ */) // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
    : m_hasError{ true } {}

    constexpr auto operator=(ResultError /* _ */) -> Result& {
        m_hasError = true;
        return *this;
    }

    [[nodiscard]] constexpr auto hasError() const -> bool {
        return m_hasError;
    }

private:
    bool m_hasError = false;
};

// Specialize for pointers
template<IsPointer T>
class [[nodiscard]] Result<T> final {
public:
    using base_type = std::remove_pointer_t<T>;

    constexpr Result() = default;

    /// Null value with defined error
    // cppcheck-suppress noExplicitConstructor
    constexpr Result(ResultError /* _ */) // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
    : m_hasError{ true } {}

    constexpr auto operator=(ResultError /* _ */) -> Result& {
        m_hasError = true;
        m_value = nullptr;
        return *this;
    }

    /// given pointer value without error
    // cppcheck-suppress noExplicitConstructor
    constexpr Result(T pointer) // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
    : m_value{ pointer } {}

    constexpr auto operator=(T pointer) -> Result& {
        m_hasError = false;
        m_value = pointer;
        return *this;
    }

    /// Downcast from derived type to base type
    template<typename U>
    friend class Result;

    template<PointerSubclassOf<T> U>
    constexpr Result(const Result<U>& other) // NOLINT(hicpp-explicit-conversions,google-explicit-constructor)
    : m_hasError{ other.m_hasError }, m_value{ other.m_value } {}

    template<PointerSubclassOf<T> U>
    constexpr auto operator=(const Result<U>& other) -> Result& {
        m_hasError = other.m_hasError;
        m_value = other.m_value;
        return *this;
    }

    [[nodiscard]] constexpr auto hasError() const -> bool {
        return m_hasError;
    }

    [[nodiscard]] constexpr auto getValue() const -> T {
        assert(not m_hasError && "Getting value from erronous Result");
        return m_value;
    }

    [[nodiscard]] constexpr auto getValueOrNull() const -> T {
        if (m_hasError) {
            return nullptr;
        }
        return m_value;
    }

    [[nodiscard]] constexpr auto operator->() const -> T {
        return getValue();
    }

    [[nodiscard]] constexpr auto operator*() const -> std::add_lvalue_reference_t<base_type> {
        return *getValue();
    }

    auto constexpr operator==(std::nullptr_t) const -> bool {
        return getValueOrNull() == nullptr;
    }

    auto constexpr operator!=(std::nullptr_t) const -> bool {
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

private:
    T m_value{};
    bool m_hasError{};
};

} // namespace lbc
