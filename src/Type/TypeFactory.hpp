//
// Created by Albert Varaksin on 21/02/2026.
//
#pragma once
#include "pch.hpp"
#include "TypeFactoryBase.hpp"
namespace lbc {
class Context;

/**
 * Factory for retrieving and creating types
 */
class TypeFactory final : public TypeFactoryBase {
public:
    explicit TypeFactory(Context& context);

    [[nodiscard]] auto getContext() -> Context& { return m_context; }

private:
    void initializeTypes();

    Context& m_context;
};
} // namespace lbc
