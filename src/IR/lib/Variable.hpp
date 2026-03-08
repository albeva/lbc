//
// Created by Albert Varaksin on 08/03/2026.
//
#pragma once
#include "pch.hpp"
#include "NamedValue.hpp"
namespace lbc {
class Symbol;
} // namespace lbc
namespace lbc::ir::lib {

/**
 * A user-declared variable (DIM x AS INTEGER).
 *
 * Represents the storage location created by a VarInstr. Each variable
 * has a name (the identifier), a type, and a reference to the frontend
 * symbol for debug and diagnostic purposes.
 */
class Variable final : public NamedValue {
public:
    Variable(Symbol* symbol);

    /** LLVM RTTI support. */
    [[nodiscard]] static auto classof(const Value* value) -> bool {
        return value->getKind() == Kind::Variable;
    }

    /** Get the frontend symbol associated with this variable. */
    [[nodiscard]] auto getSymbol() const -> Symbol* { return m_symbol; }

private:
    Symbol* m_symbol; ///< frontend symbol with type and debug info
};

} // namespace lbc::ir
