//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Operand.hpp"
namespace lbc::ir {

class Temporary : public Operand {
    constexpr Temporary(std::string name, const Type* type)
    : Operand(Kind::Temporary, std::move(name), type) {}

    [[nodiscard]] static constexpr auto classof(const Value* value) -> bool {
        return value->getKind() == Kind::Temporary;
    }
};

} // namespace lbc::ir
