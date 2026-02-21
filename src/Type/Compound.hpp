//
// Created by Albert Varaksin on 21/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Type.hpp"
namespace lbc {
class TypeFactory;

/** Bitfield flags for type qualifiers. */
enum class TypeQualifierFlags : std::uint8_t {
    None = 0U,           ///< No qualifiers
    Const = 1U << 0U,    ///< const qualifier
    Volatile = 1U << 1U, ///< volatile qualifier
};
MARK_AS_FLAGS_ENUM(TypeQualifierFlags);

/**
 * Qualified type that adds const and/or volatile qualifiers to a base type.
 */
class TypeQualified final : public Type {
protected:
    friend class TypeFactory;

    explicit constexpr TypeQualified(const Type* base, const TypeQualifierFlags flags)
    : Type(TypeKind::Qualified)
    , m_base(base)
    , m_flags(flags) { }

public:
    /** Get the unqualified base type. */
    [[nodiscard]] constexpr auto getBaseType() const -> const Type* override { return m_base; }

    /** Check if the type has a const qualifier. */
    [[nodiscard]] constexpr auto isConst() const -> bool override { return flags::has(m_flags, TypeQualifierFlags::Const); }

    /** Check if the type has a volatile qualifier. */
    [[nodiscard]] constexpr auto isVolatile() const -> bool override { return flags::has(m_flags, TypeQualifierFlags::Volatile); }

    /** Get the raw qualifier flags. */
    [[nodiscard]] constexpr auto getFlags() const -> TypeQualifierFlags { return m_flags; }

    /// LLVM RTTI support
    [[nodiscard]] constexpr static auto classof(const Type* type) -> bool {
        return type->isQualified();
    }

private:
    /// The unqualified base type
    const Type* m_base {};
    /// Qualifier flags (const, volatile)
    TypeQualifierFlags m_flags;
};

/**
 * Pointer type that points to another type (e.g. INTEGER PTR).
 */
class TypePointer final : public Type {
protected:
    friend class TypeFactory;

    explicit constexpr TypePointer(const Type* base)
    : Type(TypeKind::Pointer)
    , m_base(base) { }

public:
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
protected:
    friend class TypeFactory;

    explicit constexpr TypeReference(const Type* base)
    : Type(TypeKind::Reference)
    , m_base(base) { }

public:
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
