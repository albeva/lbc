//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "pch.hpp"
#include "NamedValue.hpp"
namespace lbc::ir {

class Temporary : public NamedValue {
public:
    Temporary(std::string name, const Type* type)
    : NamedValue(Kind::Temporary, std::move(name), type) {}

    [[nodiscard]] static auto classof(const Value* value) -> bool {
        return value->getKind() == Kind::Temporary;
    }
};

} // namespace lbc::ir
