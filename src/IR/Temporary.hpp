//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "pch.hpp"
#include "NamedValue.hpp"
namespace lbc::ir {

/**
 * A numbered temporary value (%0, %1, ...).
 *
 * Temporaries are produced by instructions that yield a result. Numbering
 * resets per function. Each temporary has a name (the number as a string)
 * and a type.
 */
class Temporary : public NamedValue {
public:
    Temporary(std::string name, const Type* type)
    : NamedValue(Kind::Temporary, std::move(name), type) {}

    /** LLVM RTTI support. */
    [[nodiscard]] static auto classof(const Value* value) -> bool {
        return value->getKind() == Kind::Temporary;
    }
};

} // namespace lbc::ir
