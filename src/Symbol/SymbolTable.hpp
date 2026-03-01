//
// Created by Albert Varaksin on 20/02/2026.
//
#pragma once
#include "pch.hpp"
namespace lbc {

/**
 * Concept for types that can be stored in a SymbolTableBase.
 * Requires a getName() method returning something convertible to llvm::StringRef.
 */
template<typename T>
concept Named = requires(const T& value) {
    { value.getName() } -> std::convertible_to<llvm::StringRef>;
};

/**
 * A scoped mapping from names to named values.
 *
 * Symbol tables form a chain via parent pointers, representing nested lexical scopes.
 * Lookups walk the chain upward by default, finding the innermost definition of a name.
 */
template<Named T>
class SymbolTableBase {
public:
    NO_COPY_AND_MOVE(SymbolTableBase)
    explicit SymbolTableBase(SymbolTableBase* parent)
    : m_parent(parent) {}
    ~SymbolTableBase() = default;

    /** Get the enclosing scope, or nullptr for the outermost scope. */
    [[nodiscard]] auto getParent() const -> SymbolTableBase* { return m_parent; }

    /**
     * Check whether a symbol with the given name exists.
     * @param id the name to look up.
     * @param recursive if true, search parent scopes as well.
     */
    [[nodiscard]] auto contains(const llvm::StringRef id, const bool recursive = true) const -> bool {
        return find(id, recursive) != nullptr;
    }

    /**
     * Find a symbol by name.
     * @param id the name to look up.
     * @param recursive if true, search parent scopes as well.
     * @return the symbol, or nullptr if not found.
     */
    [[nodiscard]] auto find(llvm::StringRef id, const bool recursive = true) const -> T* {
        if (const auto it = m_symbols.find(id); it != m_symbols.end()) {
            return it->second;
        }

        if (!recursive) {
            return nullptr;
        }

        if (m_parent != nullptr) {
            return m_parent->find(id, true);
        }

        return nullptr;
    }

    /** Insert a value into this scope. */
    void insert(T* value) {
        m_symbols.try_emplace(value->getName(), value);
    }

private:
    using Container = llvm::StringMap<T*>;

    SymbolTableBase* m_parent;
    Container m_symbols;
};

} // namespace lbc
