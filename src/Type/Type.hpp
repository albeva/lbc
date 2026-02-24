//
// Created by Albert Varaksin on 15/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Comparison.hpp"
#include "TypeBase.hpp"
namespace lbc {
class Context;
class TypeFactory;

/**
 * Base type class for the lbc type system.
 *
 * Extends the generated TypeBase with compound queries and virtual
 * methods that concrete type subclasses can override. The constructor
 * is protected — only TypeFactory (and its subclasses) can instantiate
 * types; all type objects are arena-allocated and never individually freed.
 */
class Type : public TypeBase {
public:
    // -------------------------------------------------------------------------
    // Compound type queries
    // -------------------------------------------------------------------------

    [[nodiscard]] constexpr auto isAnyPtr() const -> bool {
        return isPointer() && getBaseType()->isAny();
    }

    /** Check if the type is any integral type (signed or unsigned). */
    [[nodiscard]] constexpr auto isIntegral() const -> bool { return isSignedIntegral() || isUnsignedIntegral(); }

    /** Check if the type is any numeric type (integral or floating point). */
    [[nodiscard]] constexpr auto isNumeric() const -> bool { return isIntegral() || isFloatingPoint(); }

    // -------------------------------------------------------------------------
    // Type comparison & conversions
    // -------------------------------------------------------------------------

    /**
     * Compare this type (target) against @p from (source) for implicit convertibility.
     * Returns detailed flags describing the conversion (size change, sign change, etc.).
     */
    [[nodiscard]] auto compare(const Type* from) const -> TypeComparisonResult;

    /**
     * Find the common type between this and @p other that both can convert to.
     * References are stripped before comparison. Returns nullptr if incompatible.
     */
    [[nodiscard]] auto common(const Type* other) const -> const Type*;

    /**
     * Check if @p from can be explicitly cast to this type (AS operator).
     * More permissive than compare(): allows cross-size numeric conversions
     * (e.g. INTEGER AS BYTE) and any pointer-to-pointer casts. Does not
     * permit cross-family casts (e.g. numeric to pointer, bool to numeric).
     */
    [[nodiscard]] auto castable(const Type* from) const -> bool;

    /**
     * Strip the reference wrapper, returning the referent type.
     * Returns this unchanged if not a reference type. Used by sema to
     * work with value types — references are a storage/codegen concern.
     */
    [[nodiscard]] auto removeReference() const -> const Type*;

    // -------------------------------------------------------------------------
    // Utilities
    // -------------------------------------------------------------------------

    /** Get the underlying type for compound types (e.g. pointee, referent). */
    [[nodiscard]] constexpr virtual auto getBaseType() const -> const Type* { return nullptr; }

    /// LLVM RTTI support
    [[nodiscard]] constexpr static auto classof(const Type* /* type */) -> bool {
        return true;
    }

    [[nodiscard]] virtual auto string() const -> std::string;

protected:
    friend class TypeFactory;

    constexpr explicit Type(const TypeKind kind)
    : TypeBase(kind) { }
};

} // namespace lbc

template <>
struct std::formatter<lbc::Type, char> {
    static constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const lbc::Type& value, std::format_context& ctx) {
        return std::format_to(ctx.out(), "{}", value.string());
    }
};
