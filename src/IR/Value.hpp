//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "pch.hpp"

namespace lbc {
class Type;
}

namespace lbc::ir {

class Value {
public:
    NO_COPY_AND_MOVE(Value)

    enum class Kind : std::uint8_t {
        // Operand
        Temporary,
        Variable,
        Function,
        BasicBlock,
        // non operands
        Literal
    };

    constexpr explicit Value(const Kind kind, const Type* type)
    : m_kind(kind)
    , m_type(type) {}

    virtual ~Value() = default;

    [[nodiscard]] constexpr auto getKind() const -> Kind { return m_kind; }
    [[nodiscard]] constexpr auto getType() const -> const Type* { return m_type; }
    [[nodiscard]] static constexpr auto classof(const Value* /*unused*/) -> bool { return true; }

private:
    Kind m_kind;
    const Type* m_type;
};

} // namespace lbc::ir
