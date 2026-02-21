//
// Created by Albert Varaksin on 21/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Type.hpp"
namespace lbc {

/**
 * Qualified type that adds const and/or volatile qualifiers to a base type.
 */
class TypeQualified final : public Type {
public:
    explicit constexpr TypeQualified(const Type* base, const bool isConst, const bool isVolatile)
    : Type(TypeKind::Qualified)
    , m_base(base)
    , m_const(isConst)
    , m_volatile(isVolatile) { }

    /** Get the unqualified base type. */
    [[nodiscard]] constexpr auto getBaseType() const -> const Type* override { return m_base; }

    /** Check if the type has a const qualifier. */
    [[nodiscard]] constexpr auto isConst() const -> bool override { return m_const; }

    /** Check if the type has a volatile qualifier. */
    [[nodiscard]] constexpr auto isVolatile() const -> bool override { return m_volatile; }

    /// LLVM RTTI support
    [[nodiscard]] constexpr static auto classof(const Type* type) -> bool {
        return type->isQualified();
    }

private:
    /// The unqualified base type
    const Type* m_base {};
    /// Whether the type is const-qualified
    bool m_const;
    /// Whether the type is volatile-qualified
    bool m_volatile;
};

/**
 * Pointer type that points to another type (e.g. INTEGER PTR).
 */
class TypePointer final : public Type {
public:
    explicit constexpr TypePointer(const Type* base)
    : Type(TypeKind::Pointer)
    , m_base(base) { }

    /** Get the pointed-to type. */
    [[nodiscard]] auto getBaseType() const -> const Type* override { return m_base; }

    /// LLVM RTTI support
    [[nodiscard]] constexpr static auto classof(const Type* type) -> bool {
        return type->isPointer();
    }

private:
    /// The pointed-to type
    const Type* m_base {};
};

/**
 * Reference type that refers to another type.
 */
class TypeReference final : public Type {
public:
    explicit constexpr TypeReference(const Type* base)
    : Type(TypeKind::Reference)
    , m_base(base) { }

    /** Get the referred-to type. */
    [[nodiscard]] auto getBaseType() const -> const Type* override { return m_base; }

    /// LLVM RTTI support
    [[nodiscard]] constexpr static auto classof(const Type* type) -> bool {
        return type->isReference();
    }

private:
    /// The referred-to type
    const Type* m_base {};
};

} // namespace lbc
