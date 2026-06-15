//
// Created by Albert Varaksin on 15/06/2026.
//
#pragma once
#include "pch.hpp"
namespace lbc {

/**
 * The value category of an expression — what the expression *denotes*, as
 * opposed to its type (which describes what *kind of value* it is). Assigned
 * to every AstExpr during semantic analysis.
 *
 * Modelled on the C++ value-category system. The three primary categories map
 * onto the C++ terms as follows:
 *
 *   Addressable <-> lvalue   designates an object with a stable identity (a
 *                            storage location): it can be assigned to, have its
 *                            address taken, and be bound to a reference.
 *   Expiring    <-> xvalue   designates an object whose lifetime is ending; it
 *                            still has identity but may be moved from.
 *   Value       <-> prvalue  a pure computed value with no storage of its own
 *                            (the initialiser of an object): literals,
 *                            arithmetic results, by-value call results.
 *
 * The two C++ "union" categories are exposed as predicates rather than as
 * distinct states:
 *
 *   hasIdentity() <-> glvalue = Addressable | Expiring  (designates an object)
 *   isMovable()   <-> rvalue  = Expiring | Value        (may be moved from)
 */
struct ValueCategory final {
    /**
     * Backing value. Ordered so the C++ union categories form contiguous
     * ranges: glvalue = [Addressable, Expiring], rvalue = [Expiring, Value].
     */
    enum Kind : std::uint8_t { // NOLINT(*-use-enum-class)
        Addressable,           ///< C++ lvalue
        Expiring,              ///< C++ xvalue
        Value,                 ///< C++ prvalue
    };

    constexpr ValueCategory() = default;

    constexpr ValueCategory(const Kind kind) // NOLINT(*-explicit-conversions)
    : m_kind(kind) {}

    /** Return the underlying Kind enum. */
    [[nodiscard]] constexpr auto kind() const -> Kind { return m_kind; }

    constexpr auto operator=(const Kind kind) -> ValueCategory& {
        m_kind = kind;
        return *this;
    }

    [[nodiscard]] constexpr auto operator==(const ValueCategory& other) const -> bool = default;

    [[nodiscard]] constexpr auto operator==(const Kind kind) const -> bool {
        return m_kind == kind;
    }

    /** An Addressable expression (C++ lvalue): designates an addressable object. */
    [[nodiscard]] constexpr auto isAddressable() const -> bool { return m_kind == Addressable; }

    /** An Expiring expression (C++ xvalue): an object that may be moved from. */
    [[nodiscard]] constexpr auto isExpiring() const -> bool { return m_kind == Expiring; }

    /** A pure Value expression (C++ prvalue): a computed value with no storage. */
    [[nodiscard]] constexpr auto isValue() const -> bool { return m_kind == Value; }

    /** Designates an object with identity (C++ glvalue = Addressable or Expiring). */
    [[nodiscard]] constexpr auto hasIdentity() const -> bool { return m_kind <= Expiring; }

    /** May be moved from (C++ rvalue = Expiring or Value). */
    [[nodiscard]] constexpr auto isMovable() const -> bool { return m_kind >= Expiring; }

    /** Human-readable name for diagnostics and debugging. */
    [[nodiscard]] constexpr auto string() const -> std::string_view {
        switch (m_kind) {
        case Addressable:
            return "addressable";
        case Expiring:
            return "expiring";
        case Value:
            return "value";
        }
        return {};
    }

private:
    Kind m_kind = Value;
};

} // namespace lbc
