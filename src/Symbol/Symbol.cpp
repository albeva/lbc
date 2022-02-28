//
// Created by Albert on 27/02/2022.
//
#include "Symbol.hpp"
#include "Type/TypeProxy.hpp"
using namespace lbc;

const TypeRoot* Symbol::getType() const noexcept {
    if (m_typeProxy == nullptr) {
        return nullptr;
    }
    return m_typeProxy->getType();
}