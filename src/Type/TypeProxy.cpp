//
// Created by Albert on 28/02/2022.
//
#include "TypeProxy.hpp"
#include "Type.hpp"
using namespace lbc;

const TypeRoot* TypeProxy::getType() noexcept {
    const auto* base = getBaseType();
    if (m_dereference > 0 && base != nullptr) {
        for (int i = 0; i < m_dereference; i++) {
            base = base->getPointer(*m_context);
        }
        m_storage = base;
        m_dereference = 0;
        m_context = nullptr;
    }
    return base;
}

const TypeRoot* TypeProxy::getBaseType() noexcept {
    if (std::holds_alternative<const TypeRoot*>(m_storage)) {
        return std::get<const TypeRoot*>(m_storage);
    }
    if (auto* proxy = getNestedProxy()) {
        if (const auto* type = proxy->getType()) {
            m_storage = type;
            return type;
        }
    }
    return nullptr;
}

TypeProxy* TypeProxy::getNestedProxy() const noexcept {
    if (std::holds_alternative<TypeProxy*>(m_storage)) {
        return std::get<TypeProxy*>(m_storage);
    }
    return nullptr;
}
