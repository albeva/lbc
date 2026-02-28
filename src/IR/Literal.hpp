//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Symbol/LiteralValue.hpp"
#include "Value.hpp"
namespace lbc::ir {

class Literal : public Value {
public:
    Literal(const Type* type, const LiteralValue& value)
    : Value(Kind::Literal, type)
    , m_value(value) {}

    [[nodiscard]] auto getValue() const -> const LiteralValue& { return m_value; }
    auto setValue(const LiteralValue& value) { m_value = value; }

    [[nodiscard]] static auto classof(const Value* value) -> bool {
        return value->getKind() == Kind::Literal;
    }

private:
    LiteralValue m_value;
};

} // namespace lbc::ir
