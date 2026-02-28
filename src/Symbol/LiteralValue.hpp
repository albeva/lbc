//
// Created by Albert Varaksin on 13/02/2026.
//
#pragma once
#include "pch.hpp"

namespace lbc {

/**
 * Type-safe container for a compile-time literal value.
 *
 * Stores values parsed from source as one of a fixed set of canonical types:
 * bool, uint64_t, double, or StringRef. Arithmetic types are widened to the
 * canonical storage type on construction and narrowed back on retrieval via
 * from<T>() and get<T>().
 */
class LiteralValue final {
public:
    /// Canonical storage type for all integer literals.
    using Integral = std::uint64_t;
    /// Canonical storage type for all floating-point literals.
    using FloatingPoint = double;
    /// Canonical storage type for string literals.
    using String = llvm::StringRef;

    /// The underlying variant holding the literal value, or monostate for null.
    using Value = std::variant<std::monostate, bool, Integral, FloatingPoint, String>;

    /** Construct a null literal value. */
    LiteralValue() = default;

    /**
     * Construct a LiteralValue from an arbitrary source type.
     * Integral types are widened to uint64_t, floating-point types to double.
     */
    template <typename T>
    [[nodiscard]] static constexpr auto from(const T& value) -> LiteralValue {
        if constexpr (std::is_same_v<T, bool>) { // NOLINT(*-branch-clone)
            return LiteralValue { value };
        } else if constexpr (std::is_integral_v<T>) {
            return LiteralValue { static_cast<Integral>(value) };
        } else if constexpr (std::is_floating_point_v<T>) {
            return LiteralValue { static_cast<FloatingPoint>(value) };
        } else if constexpr (std::is_same_v<T, String>) {
            return LiteralValue { value };
        } else if constexpr (std::is_same_v<T, std::monostate>) {
            return LiteralValue { value };
        } else {
            std::unreachable();
        }
    }

    /**
     * Retrieve the stored value, casting from the canonical storage type to T.
     * The variant must currently hold the corresponding canonical type.
     */
    template <typename T>
    [[nodiscard]] constexpr auto get() const -> T {
        if constexpr (std::is_same_v<T, bool>) {
            return std::get<bool>(m_value);
        } else if constexpr (std::is_integral_v<T>) {
            return static_cast<T>(std::get<Integral>(m_value));
        } else if constexpr (std::is_floating_point_v<T>) {
            return static_cast<T>(std::get<FloatingPoint>(m_value));
        } else if constexpr (std::is_same_v<T, String>) {
            return std::get<String>(m_value);
        } else if constexpr (std::is_same_v<T, std::monostate>) {
            return std::get<std::monostate>(m_value);
        } else {
            std::unreachable();
        }
    }

    /**
     * Replace the stored value. Accepts any supported source type,
     * as well as LiteralValue and Value for direct assignment.
     */
    template <typename T>
    constexpr void set(const T& value) {
        if constexpr (std::is_same_v<T, bool>) { // NOLINT(*-branch-clone)
            m_value = value;
        } else if constexpr (std::is_integral_v<T>) {
            m_value = static_cast<Integral>(value);
        } else if constexpr (std::is_floating_point_v<T>) {
            m_value = static_cast<FloatingPoint>(value);
        } else if constexpr (std::is_same_v<T, String>) {
            m_value = value;
        } else if constexpr (std::is_same_v<T, std::monostate>) {
            m_value = value;
        } else if constexpr (std::is_same_v<T, LiteralValue>) {
            m_value = value.m_value;
        } else if constexpr (std::is_same_v<T, Value>) {
            m_value = value;
        } else {
            std::unreachable();
        }
    }

    /** Try to retrieve the value as T, returning nullopt if the type doesn't match. */
    template <typename T>
    [[nodiscard]] constexpr auto as() const -> std::optional<T> {
        if (is<T>()) {
            return get<T>();
        }
        return std::nullopt;
    }

    [[nodiscard]] constexpr auto isNull() const -> bool { return is<std::monostate>(); }
    [[nodiscard]] constexpr auto isBool() const -> bool { return is<bool>(); }
    [[nodiscard]] constexpr auto isIntegral() const -> bool { return is<Integral>(); }
    [[nodiscard]] constexpr auto isFloatingPoint() const -> bool { return is<FloatingPoint>(); }
    [[nodiscard]] constexpr auto isString() const -> bool { return is<String>(); }

    /** Get the underlying variant directly. */
    [[nodiscard]] constexpr auto storage() const -> const Value& { return m_value; }

    /** Convert the stored value to its string representation. */
    [[nodiscard]] auto asString() const -> std::string {
        static auto visitor = Visitor {
            [](std::monostate) static { return "null"s; },
            [](const String value) static { return value.str(); },
            [](const Integral value) static { return std::to_string(value); },
            [](const FloatingPoint value) static { return std::to_string(value); },
            [](const bool value) static { return value ? "true"s : "false"s; }
        };
        return std::visit(visitor, m_value);
    }

private:
    template <typename T>
    constexpr explicit LiteralValue(const T value)
    : m_value(value) {}

    template <typename T>
    [[nodiscard]] constexpr auto is() const -> bool {
        if constexpr (std::is_same_v<T, bool>) {
            return std::holds_alternative<bool>(m_value);
        } else if constexpr (std::is_integral_v<T>) {
            return std::holds_alternative<std::uint64_t>(m_value);
        } else if constexpr (std::is_floating_point_v<T>) {
            return std::holds_alternative<FloatingPoint>(m_value);
        } else if constexpr (std::is_same_v<T, String>) {
            return std::holds_alternative<String>(m_value);
        } else if constexpr (std::is_same_v<T, std::monostate>) {
            return std::holds_alternative<std::monostate>(m_value);
        } else {
            std::unreachable();
        }
    }

    Value m_value;
};

} // namespace lbc

template <>
struct std::formatter<lbc::LiteralValue, char> {
    static constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const lbc::LiteralValue& value, std::format_context& ctx) {
        return std::format_to(ctx.out(), "{}", value.asString());
    }
};
