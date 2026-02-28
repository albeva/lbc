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
        // Named values
        Temporary,
        Variable,
        Function,
        // Block kinds
        BasicBlock,
        ScopedBlock,
        // non-named values
        Literal
    };

    Value(const Kind kind, const Type* type)
    : m_kind(kind)
    , m_type(type) {}

    [[nodiscard]] auto getKind() const -> Kind { return m_kind; }
    [[nodiscard]] auto getType() const -> const Type* { return m_type; }
    [[nodiscard]] static auto classof(const Value* /*unused*/) -> bool { return true; }

private:
    Kind m_kind;
    const Type* m_type;
};

} // namespace lbc::ir
