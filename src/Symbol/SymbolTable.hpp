//
// Created by Albert Varaksin on 06/07/2020.
//
#pragma once
#include "pch.hpp"
#include "Symbol.hpp"

namespace lbc {
class Context;
struct AstFuncDecl;

class SymbolTable final {
    using Container = llvm::StringMap<Symbol*>;

public:
    NO_COPY_AND_MOVE(SymbolTable)
    explicit SymbolTable(SymbolTable* parent = nullptr, AstFuncDecl* function = nullptr)
    : m_parent{ parent }, m_function{ function } {}

    ~SymbolTable() = default;

    [[nodiscard]] auto getParent() const -> SymbolTable* { return m_parent; }
    void setParent(SymbolTable* parent) { m_parent = parent; }

    [[nodiscard]] auto getFunction() const -> AstFuncDecl* {
        if (m_function == nullptr && m_parent != nullptr) {
            return m_parent->getFunction();
        }
        return m_function;
    }

    void insert(Symbol* symbol);
    void import(SymbolTable* table);

    [[nodiscard]] auto exists(llvm::StringRef name, bool recursive = false) const -> bool {
        return find(name, recursive) != nullptr;
    }
    [[nodiscard]] auto find(llvm::StringRef name, bool recursive = true) const -> Symbol*;
    [[nodiscard]] auto getSymbols() const -> std::vector<Symbol*>;

    [[nodiscard]] auto size() const { return m_symbols.size(); }

    // Make vanilla new/delete illegal.
    auto operator new(size_t) -> void* = delete;
    void operator delete(void*) = delete;

    // Allow placement new
    auto operator new(size_t size, void* ptr) -> void* {
        assert(size >= sizeof(SymbolTable));
        assert(ptr);
        return ptr;
    }

private:
    SymbolTable* m_parent;
    AstFuncDecl* m_function;
    Container m_symbols;
    std::vector<SymbolTable*> m_imports;
};

} // namespace lbc
