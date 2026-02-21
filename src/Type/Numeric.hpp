//
// Created by Albert Varaksin on 21/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Type.hpp"
namespace lbc {

/**
 * Integral type representing signed and unsigned integer types.
 *
 * Covers Byte, Short, Integer, Long and their unsigned variants.
 */
class TypeIntegral final : public Type {
public:
    constexpr TypeIntegral(const TypeKind kind, const std::uint8_t size, const bool isSigned)
    : Type(kind)
    , m_size(size)
    , m_signed(isSigned) { }

    /** Get the size of this type in bytes. */
    [[nodiscard]] constexpr auto getBytes() const -> std::size_t { return m_size; }

    /** Get the size of this type in bits. */
    [[nodiscard]] constexpr auto getBits() const -> std::size_t { return getBytes() * 8; }

    /** Check if this is a signed integral type. */
    [[nodiscard]] constexpr auto isSigned() const -> bool { return m_signed; }

    /** Get the signed counterpart of this integral type. */
    [[nodiscard]] /* constexpr */ auto getSigned() const -> const TypeIntegral*;

    /** Get the unsigned counterpart of this integral type. */
    [[nodiscard]] /* constexpr */ auto getUnsigned() const -> const TypeIntegral*;

    /// LLVM RTTI support
    [[nodiscard]] constexpr static auto classof(const Type* type) -> bool {
        return type->isIntegral();
    }

private:
    /// Size in bytes
    std::uint8_t m_size {};
    /// Whether this is a signed type
    bool m_signed;
};

/**
 * Floating point type representing Single and Double precision values.
 */
class TypeFloatingPoint final : public Type {
public:
    constexpr TypeFloatingPoint(const TypeKind kind, const std::uint8_t size)
    : Type(kind)
    , m_size(size) { }

    /** Get the size of this type in bytes. */
    [[nodiscard]] constexpr auto getBytes() const -> std::size_t { return m_size; }

    /** Get the size of this type in bits. */
    [[nodiscard]] constexpr auto getBits() const -> std::size_t { return getBytes() * 8; }

    /// LLVM RTTI support
    [[nodiscard]] constexpr static auto classof(const Type* type) -> bool {
        return type->isFloatingPoint();
    }

private:
    /// Size in bytes
    std::uint8_t m_size {};
};

} // namespace lbc
