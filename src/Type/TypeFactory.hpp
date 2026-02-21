//
// Created by Albert Varaksin on 21/02/2026.
//
#pragma once
#include "pch.hpp"
#include "TypeFactoryBase.hpp"
namespace lbc {
class Context;

/**
 * Factory for retrieving and creating types.
 *
 * Extends the generated TypeFactoryBase with arena-allocated type
 * construction. Singleton types (primitives, integrals, floats, sentinels)
 * are created once during construction and accessed via inherited getters.
 * Compound and aggregate types are created on demand.
 */
class TypeFactory final : public TypeFactoryBase {
public:
    /** Construct the factory and initialize all singleton types. */
    explicit TypeFactory(Context& context);

    /** Get the owning context. */
    [[nodiscard]] auto getContext() -> Context& { return m_context; }

private:
    /** Create and register all singleton type instances. */
    void initializeTypes();

    /// The owning context providing arena allocation
    Context& m_context;
};
} // namespace lbc
