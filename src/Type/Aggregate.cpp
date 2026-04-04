//
// Created by Albert Varaksin on 24/02/2026.
//
#include "Aggregate.hpp"
using namespace lbc;

auto TypeFunction::string() const -> std::string {
    std::string out;
    if (m_returnType->isVoid()) {
        out = "SUB(";
    } else {
        out = "FUNCTION(";
    }

    Joiner commas { out };
    for (const auto* param : m_params) {
        commas();
        out += param->string();
    }

    if (m_returnType->isVoid()) {
        out += ")";
    } else {
        out += ") AS ";
        out += m_returnType->string();
    }

    return out;
}
