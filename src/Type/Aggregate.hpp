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

    /// LLVM RTTI support
    [[nodiscard]] constexpr static auto classof(const Type* type) -> bool {
        return type->isFunction();
    }

protected:
    friend class TypeFactory;

    explicit constexpr TypeFunction(
        const std::span<const Type*> params,
        const Type* returnType
    )
    : Type(TypeKind::Function)
    , m_params(params)
    , m_returnType(returnType) { }

private:
    /// Parameter types
    std::span<const Type*> m_params;
    /// Return type
    const Type* m_returnType;
};

} // namespace lbc
