//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Value.hpp"
namespace lbc::ir {

/**
 * A Value with a name.
 *
 * Intermediate base for all IR values that carry a textual name —
 * temporaries (%0), variables, functions, and blocks. Satisfies the
 * Named concept so NamedValues can be stored in a SymbolTableBase.
 */
class NamedValue : public Value {
public:
    /** LLVM RTTI support. */
    [[nodiscard]] static auto classof(const Value* value) -> bool {
        return value->getKind() >= Kind::Temporary && value->getKind() <= Kind::ScopedBlock;
    }

    /** Get the name of this value. */
    [[nodiscard]] auto getName() const -> const std::string& { return m_name; }

protected:
    NamedValue(const Kind kind, std::string name, const Type* type)
    : Value(kind, type)
    , m_name(std::move(name)) {}

private:
    std::string m_name; ///< the name of this value
};

} // namespace lbc::ir
