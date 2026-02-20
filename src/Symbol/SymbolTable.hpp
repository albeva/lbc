//
// Created by Albert Varaksin on 20/02/2026.
//
#pragma once
#include "pch.hpp"
namespace lbc {
class Symbol;
class Context;

/**
 * A scoped mapping from names to symbols.
 *
 * Symbol tables form a chain via parent pointers, representing nested lexical scopes.
 * Lookups walk the chain upward by default, finding the innermost definition of a name.
 */
class SymbolTable final {
public:
    NO_COPY_AND_MOVE(SymbolTable)
    explicit SymbolTable(SymbolTable* parent);
    ~SymbolTable();

    /** Get the enclosing scope, or nullptr for the outermost scope. */
    [[nodiscard]] auto getParent() const -> SymbolTable* { return m_parent; }

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
    [[nodiscard]] auto find(llvm::StringRef id, bool recursive = true) const -> Symbol*;

    /** Insert a symbol into this scope. */
    void insert(Symbol* symbol);

private:
    using Container = llvm::StringMap<Symbol*>;

    SymbolTable* m_parent;
    Container m_symbols;
};

} // namespace lbc
