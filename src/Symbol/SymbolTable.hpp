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
    explicit SymbolTable(SymbolTable* parent = nullptr, AstFuncDecl* function = nullptr) noexcept
    : m_parent{ parent }, m_function{ function } {}

    ~SymbolTable() noexcept = default;

    [[nodiscard]] SymbolTable* getParent() const noexcept { return m_parent; }
    void setParent(SymbolTable* parent) noexcept { m_parent = parent; }

    [[nodiscard]] AstFuncDecl* getFunction() const noexcept {
        if (m_function == nullptr && m_parent != nullptr) {
            return m_parent->getFunction();
        }
        return m_function;
    }

    void insert(Symbol* symbol) noexcept;
    void import(SymbolTable* table);

    [[nodiscard]] bool exists(llvm::StringRef name, bool recursive = false) const noexcept {
        return find(name, recursive) != nullptr;
    }
    [[nodiscard]] Symbol* find(llvm::StringRef name, bool recursive = true) const noexcept;
    [[nodiscard]] std::vector<Symbol*> getSymbols() const;

    [[nodiscard]] auto size() const noexcept { return m_symbols.size(); }

    // Make vanilla new/delete illegal.
    void* operator new(size_t) = delete;
    void operator delete(void*) = delete;

    // Allow placement new
    void* operator new(size_t /*size*/, void* ptr) {
        assert(ptr);
        return ptr;
    }

private:
    SymbolTable* m_parent;
    AstFuncDecl* m_function;
    Container m_symbols{};
    std::vector<SymbolTable*> m_imports{};
};

} // namespace lbc
