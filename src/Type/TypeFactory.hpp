//
// Created by Albert Varaksin on 21/02/2026.
//
#pragma once
#include "pch.hpp"
namespace lbc {
class Context;
class Type;
class TypeIntegral;
class TypeFloatingPoint;
class TypePointer;
class TypeReference;
class TypeQualified;
class TypeFunction;

/**
 * Factory for creating and retrieving types
 */
class TypeFactory final {
public:
    NO_COPY_AND_MOVE(TypeFactory)
    explicit TypeFactory(Context& context);

    [[nodiscard]] auto getContext() -> Context& { return m_context; }

private:
    Context& m_context;
};
} // namespace lbc
