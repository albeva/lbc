//
// Created by Albert Varaksin on 20/02/2026.
//
#include "SymbolTable.hpp"
#include "Symbol.hpp"
using namespace lbc;

SymbolTable::SymbolTable(SymbolTable* parent)
: m_parent(parent) {
}

SymbolTable::~SymbolTable() = default;

auto SymbolTable::find(const llvm::StringRef id, bool recursive) const -> Symbol* {
    // find in current table
    if (const auto it = m_symbols.find(id); it != m_symbols.end()) {
        return it->second;
    }

    // if recursive search is not requested, return nullptr
    if (!recursive) {
        return nullptr;
    }

    // search in parent symbol table
    if (m_parent != nullptr) {
        return m_parent->find(id, true);
    }

    // not found
    return nullptr;
}

void SymbolTable::insert(Symbol* symbol) {
    m_symbols.try_emplace(symbol->getName(), symbol);
}
