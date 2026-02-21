//
// Created by Albert Varaksin on 15/02/2026.
//
#pragma once
#include "pch.hpp"
#include "TypeBase.hpp"
namespace lbc {
class Context;

/**
 * Base type class for the lbc type system.
 *
 * Extends the generated TypeBase with compound queries and virtual
 * methods that concrete type subclasses can override.
 */
class Type : public TypeBase {
public:
    using TypeBase::TypeBase;

    // -------------------------------------------------------------------------
    // Compound type queries
    // -------------------------------------------------------------------------

    /** Check if the type is any integral type (signed or unsigned). */
    [[nodiscard]] constexpr auto isIntegral() const -> bool { return isSignedIntegral() || isUnsignedIntegral(); }

    /** Check if the type is any numeric type (integral or floating point). */
    [[nodiscard]] constexpr auto isNumeric() const -> bool { return isIntegral() || isFloatingPoint(); }

    /** Check if the type has a const qualifier. */
    [[nodiscard]] constexpr virtual auto isConst() const -> bool { return false; }

    /** Check if the type has a volatile qualifier. */
    [[nodiscard]] constexpr virtual auto isVolatile() const -> bool { return false; }

    // -------------------------------------------------------------------------
    // Utilities
    // -------------------------------------------------------------------------

    /** Get the underlying type for compound types (e.g. pointee, referent). */
    [[nodiscard]] constexpr virtual auto getBaseType() const -> const Type* { return nullptr; }

    /// LLVM RTTI support
    [[nodiscard]] constexpr static auto classof(const Type* /* type */) -> bool {
        return true;
    }
};

} // namespace lbc
