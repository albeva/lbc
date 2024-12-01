//
// Created by Albert Varaksin on 29/11/2024.
//
#pragma once
#include "pch.hpp"

namespace lbc {
namespace detail {
    using TokenValueBase = std::variant<std::monostate, llvm::StringRef, std::uint64_t, double, bool>;
}

/**
 * \struct TokenValue
 * \brief Inherits from detail::TokenValueType and provides type aliases for various token types.
 *
 * This struct is a final class that extends the TokenValueBase from the detail namespace.
 * It provides type aliases for integral, floating point, null, and string types.
 */
struct TokenValue final : detail::TokenValueBase {
    using detail::TokenValueBase::variant;

    using IntegralType = std::uint64_t;
    using FloatingPointType = double;
    using NullType = std::monostate;
    using StringType = llvm::StringRef;

    template <typename T>
    constexpr static auto from(const T& value) -> TokenValue {
        TokenValue res;
        res.set(value);
        return res;
    }

    /**
     * \brief Retrieve the value of the token as the specified type.
     *
     * \tparam T The type to which the token value should be converted.
     * \return The token value converted to the specified type.
     */
    template <typename T>
    [[nodiscard]] constexpr auto get() const -> T {
        if constexpr (std::is_same_v<T, bool>) {
            return getBoolean();
        } else if constexpr (std::is_integral_v<T>) {
            return static_cast<T>(getIntegral());
        } else if constexpr (std::is_floating_point_v<T>) {
            return static_cast<T>(getFloatingPoint());
        } else if constexpr (std::is_same_v<T, StringType>) {
            return getString();
        } else if constexpr (std::is_same_v<T, NullType>) {
            return NullType {};
        } else {
            llvm_unreachable("Unsupported type");
        }
    }

    /**
     * \brief Retrieve the value of the token as the specified type.
     *
     * \tparam T The type to which the token value should be converted.
     * \return The token value converted to the specified type.
     */
    template <typename T>
    constexpr void set(const T& value) {
        if constexpr (std::is_same_v<T, bool>) {
            return setBoolean(value);
        } else if constexpr (std::is_integral_v<T>) {
            return setIntegral(value);
        } else if constexpr (std::is_floating_point_v<T>) {
            return setFloatingPoint(value);
        } else if constexpr (std::is_same_v<T, StringType>) {
            return setString(value);
        } else if constexpr (std::is_same_v<T, NullType>) {
            *this = NullType {};
            return;
        } else {
            llvm_unreachable("Unsupported type");
        }
    }

    /**
     * Check if the token value is an integral type.
     */
    [[nodiscard]] constexpr auto isIntegral() const -> bool {
        return std::holds_alternative<IntegralType>(*this);
    }

    /**
     * Get the integral value. This function should only be called if isIntegral() returns true.
     */
    [[nodiscard]] constexpr auto getIntegral() const -> IntegralType {
        return std::get<IntegralType>(*this);
    }

    /**
     * Set the integral value.
     */
    constexpr void setIntegral(std::integral auto value) {
        *this = static_cast<IntegralType>(value);
    }

    /**
     * Check if the token value is a floating point type.
     */
    [[nodiscard]] constexpr auto isFloatingPoint() const -> bool {
        return std::holds_alternative<FloatingPointType>(*this);
    }

    /**
     * Get the floating point value. This function should only be called if isFloatingPoint() returns true.
     */
    [[nodiscard]] constexpr auto getFloatingPoint() const -> FloatingPointType {
        return std::get<FloatingPointType>(*this);
    }

    /**
     * Set the floating point value.
     */
    constexpr void setFloatingPoint(std::floating_point auto value) {
        *this = static_cast<FloatingPointType>(value);
    }

    /**
     * Check if the token value is a null type.
     */
    [[nodiscard]] constexpr auto isNull() const -> bool {
        return std::holds_alternative<NullType>(*this);
    }

    /**
     * Check if the token value is a string type.
     */
    [[nodiscard]] constexpr auto isString() const -> bool {
        return std::holds_alternative<StringType>(*this);
    }

    /**
     * Get the string value. This function should only be called if isString() returns true.
     */
    [[nodiscard]] constexpr auto getString() const -> StringType {
        return std::get<StringType>(*this);
    }

    /**
     * Set the string value.
     */
    constexpr void setString(StringType value) {
        *this = value;
    }

    /**
     * Check if the token value is a boolean type.
     */
    [[nodiscard]] constexpr auto isBoolean() const -> bool {
        return std::holds_alternative<bool>(*this);
    }

    /**
     * Get the boolean value. This function should only be called if isBoolean() returns true.
     */
    [[nodiscard]] constexpr auto getBoolean() const -> bool {
        return std::get<bool>(*this);
    }

    /**
     * Set the boolean value.
     */
    constexpr void setBoolean(bool value) {
        *this = value;
    }

    /**
     * Convert the token value to a string.
     */
    [[nodiscard]] constexpr auto asString() const -> std::string {
        constexpr auto visitor = Visitor {
            [](NullType) { return "NULL"s; },
            [](const StringType value) { return value.str(); },
            [](const IntegralType value) { return std::to_string(value); },
            [](const FloatingPointType value) { return std::to_string(value); },
            [](const bool value) { return value ? "TRUE"s : "FALSE"s; }
        };
        return std::visit(visitor, *this);
    }
};

} // namespace lbc
