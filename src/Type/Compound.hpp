//
// Created by Albert Varaksin on 21/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Type.hpp"
namespace lbc {
class TypeFactory;

/**
 * Pointer type that points to another type (e.g. INTEGER PTR).
 */
class TypePointer final : public Type {
public:
    /** Get the pointed-to type. */
    [[nodiscard]] auto getBaseType() const -> const Type* override { return m_base; }

    /// LLVM RTTI support
    [[nodiscard]] constexpr static auto classof(const Type* type) -> bool {
        return type->isPointer();
    }

    /// Get type string
    [[nodiscard]] auto string() const -> std::string override;

protected:
    friend class TypeFactory;

    explicit constexpr TypePointer(const Type* base)
    : Type(TypeKind::Pointer)
    , m_base(base) { }

private:
    /// The pointed-to type
    const Type* m_base {};
};

/**
 * Reference type that refers to another type.
 */
class TypeReference final : public Type {
public:
    /** Get the referred-to type. */
    [[nodiscard]] auto getBaseType() const -> const Type* override { return m_base; }

    /// LLVM RTTI support
    [[nodiscard]] constexpr static auto classof(const Type* type) -> bool {
        return type->isReference();
    }

    /// Get type string
    [[nodiscard]] auto string() const -> std::string override;

protected:
    friend class TypeFactory;

    explicit constexpr TypeReference(const Type* base)
    : Type(TypeKind::Reference)
    , m_base(base) { }

private:
    /// The referred-to type
    const Type* m_base {};
};

} // namespace lbc
