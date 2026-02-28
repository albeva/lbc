//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Symbol/LiteralValue.hpp"
#include "Value.hpp"
namespace lbc::ir {

/**
 * A compile-time literal value (integer, float, bool, string, null).
 *
 * Literals are unnamed values that appear as immediate operands to
 * instructions. They carry both a type and a LiteralValue.
 */
class Literal : public Value {
public:
    Literal(const Type* type, const LiteralValue& value)
    : Value(Kind::Literal, type)
    , m_value(value) {}

    /** Get the literal value. */
    [[nodiscard]] auto getValue() const -> const LiteralValue& { return m_value; }
    /** Set the literal value. */
    auto setValue(const LiteralValue& value) { m_value = value; }

    /** LLVM RTTI support. */
    [[nodiscard]] static auto classof(const Value* value) -> bool {
        return value->getKind() == Kind::Literal;
    }

private:
    LiteralValue m_value; ///< the literal value
};

} // namespace lbc::ir
