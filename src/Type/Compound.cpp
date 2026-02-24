//
// Created by Albert Varaksin on 24/02/2026.
//
#include "Compound.hpp"
using namespace lbc;

auto TypePointer::string() const -> std::string {
    return m_base->string() + " PTR";
}

auto TypeReference::string() const -> std::string {
    return m_base->string() + " REF";
}
