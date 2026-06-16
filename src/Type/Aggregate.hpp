//
// Created by Albert Varaksin on 21/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Type.hpp"
namespace lbc {
class TypeFactory;

/**
 * Function type representing a callable signature with parameter types
 * and a return type.
 */
class TypeFunction final : public Type {
public:
    /** Get the parameter types. */
    [[nodiscard]] auto getParams() const -> std::span<const Type*> { return m_params; }

    /** Get the return type. */
    [[nodiscard]] auto getReturnType() const -> const Type* { return m_returnType; }

    /** Whether the function takes trailing C-style variadic arguments (`...`). */
    [[nodiscard]] constexpr auto isVariadic() const -> bool { return m_variadic; }

    /// LLVM RTTI support
    [[nodiscard]] constexpr static auto classof(const Type* type) -> bool {
        return type->isFunction();
    }

    /// Get type string representation
    [[nodiscard]] auto string() const -> std::string override;

protected:
    friend class TypeFactory;

    explicit constexpr TypeFunction(
        const std::span<const Type*> params,
        const Type* returnType,
        const bool variadic
    )
    : Type(TypeKind::Function)
    , m_params(params)
    , m_returnType(returnType)
    , m_variadic(variadic) {}

private:
    /// Parameter types
    std::span<const Type*> m_params;
    /// Return type
    const Type* m_returnType;
    /// Whether trailing C-style variadic arguments are accepted
    bool m_variadic;
};

} // namespace lbc
