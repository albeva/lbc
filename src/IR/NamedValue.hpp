//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Value.hpp"
namespace lbc::ir {

class NamedValue : public Value {
public:
    [[nodiscard]] static auto classof(const Value* value) -> bool {
        return value->getKind() >= Kind::Temporary && value->getKind() <= Kind::ScopedBlock;
    }

    [[nodiscard]] auto getName() const -> const std::string& { return m_name; }

protected:
    NamedValue(const Kind kind, std::string name, const Type* type)
    : Value(kind, type)
    , m_name(std::move(name)) {}

private:
    std::string m_name;
};

} // namespace lbc::ir
