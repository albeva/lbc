//
// Created by Albert Varaksin on 15/02/2026.
//
#pragma once
#include "pch.hpp"
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
     * Find the common type between this and @p other that both can implicitly
     * convert to. Returns the wider type on success, nullptr if incompatible.
     */
    [[nodiscard]] auto common(const Type* other) const -> const Type*;

    /** Conversion mode for convertible(). */
    enum class Conversion : std::uint8_t {
        /// Safe widening only (e.g. BYTE -> SHORT, ptr -> ANY PTR).
        Implicit,
        /// Explicit cast via AS operator (e.g. LONG -> BYTE, DOUBLE -> INTEGER).
        Cast
    };

    /**
     * Check if @p from can be converted to this type under the given @p mode.
     *
     * Implicit: allows widening within the same numeric family and safe pointer
     * conversions (null -> ptr, typed ptr -> ANY PTR).
     *
     * Cast: allows any numeric-to-numeric and any pointer-to-pointer conversion,
     * but not cross-family (e.g. numeric to pointer).
     *
     * Identity (this == from) is always true regardless of mode.
     */
    [[nodiscard]] auto convertible(const Type* from, Conversion mode) const -> bool;

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
    : TypeBase(kind) {}
};

} // namespace lbc

template<>
struct std::formatter<lbc::Type, char> {
    static constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const lbc::Type& value, std::format_context& ctx) {
        return std::format_to(ctx.out(), "{}", value.string());
    }
};
