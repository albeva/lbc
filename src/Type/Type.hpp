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

    [[nodiscard]] auto compare(const Type* from) const -> TypeComparisonResult;
    [[nodiscard]] auto common(const Type* other) const -> const Type*;

    // -------------------------------------------------------------------------
    // Utilities
    // -------------------------------------------------------------------------

    /** Get the underlying type for compound types (e.g. pointee, referent). */
    [[nodiscard]] constexpr virtual auto getBaseType() const -> const Type* { return nullptr; }

    /// LLVM RTTI support
    [[nodiscard]] constexpr static auto classof(const Type* /* type */) -> bool {
        return true;
    }

protected:
    friend class TypeFactory;

    constexpr explicit Type(const TypeKind kind)
    : TypeBase(kind) { }
};

} // namespace lbc
