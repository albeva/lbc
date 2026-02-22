//
// Created by Albert Varaksin on 21/02/2026.
//
#pragma once
#include "pch.hpp"
#include "TypeFactoryBase.hpp"
#include "llvm/ADT/SmallVector.h"
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

    /** Get the pre-created ANY PTR type (equivalent to C void*). */
    [[nodiscard]] auto getAnyPtr() const -> const TypePointer* { return m_anyPtr; }

    /** Get or create a pointer type to the given base type. */
    [[nodiscard]] auto getPointer(const Type* type) -> const TypePointer*;

    /** Get or create a reference type to the given base type. */
    [[nodiscard]] auto getReference(const Type* type) -> const TypeReference*;

    /** Get or create a function type with the given parameter and return types. */
    [[nodiscard]] auto getFunction(std::span<const Type*> params, const Type* returnType) -> const TypeFunction*;

    /** Get the owning context. */
    [[nodiscard]] auto getContext() const -> Context& { return m_context; }

private:

    /** Allocate raw memory from the arena. */
    [[nodiscard]] auto allocate(std::size_t size, std::size_t alignment) const -> void*;

    /**
     * Construct a type in arena-allocated memory.
     *
     * Types are never individually freed; the arena owns their lifetime.
     */
    template <std::derived_from<Type> T, typename... Args>
    [[nodiscard]] auto create(Args&&... args) -> const T* {
        void* addr = allocate(sizeof(T), alignof(T));
        new (addr) T(std::forward<Args>(args)...);
        return static_cast<const T*>(addr);
    }

    /** Create and register all singleton type instances. */
    void createSingletonTypes();

    // NOLINTNEXTLINE(*-avoid-const-or-ref-data-members)
    Context& m_context;             ///< The owning context providing arena allocation
    const TypePointer* m_anyPtr {}; ///< Any Ptr is frequent, so pre-create it

    // -------------------------------------------------------------------------
    // Function type cache (keyed by pre-computed hash, buckets for collisions)
    // -------------------------------------------------------------------------

    /// Identity hasher for pre-computed llvm::hash_code values
    struct FunctionKeyHash final {
        static auto operator()(const llvm::hash_code& key) -> std::size_t {
            return key;
        }
    };
    using FunctionMap = std::unordered_map<llvm::hash_code, llvm::SmallVector<const TypeFunction*, 2>, FunctionKeyHash>;
    FunctionMap m_functions; ///< Cached function types, keyed by hash of return type + params

    // -------------------------------------------------------------------------
    // Compound type caches (keyed by base type pointer)
    // -------------------------------------------------------------------------

    template <typename T>
    using Map = std::unordered_map<const Type*, T>;
    Map<const TypePointer*> m_pointers;     ///< Cached pointer types
    Map<const TypeReference*> m_references; ///< Cached reference types
};
} // namespace lbc
