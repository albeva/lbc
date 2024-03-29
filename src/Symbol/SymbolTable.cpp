//
// Created by Albert Varaksin on 06/07/2020.
//
#include "SymbolTable.hpp"
#include "Driver/Context.hpp"
#include "Symbol.hpp"
using namespace lbc;

void SymbolTable::insert(Symbol* symbol) {
    m_symbols.insert({ symbol->name(), symbol });
}

void SymbolTable::import(SymbolTable* table) {
    m_imports.push_back(table);
}

Symbol* SymbolTable::find(llvm::StringRef name, bool recursive) const {
    if (auto iter = m_symbols.find(name); iter != m_symbols.end()) {
        return iter->second;
    }

    if (not recursive) {
        return nullptr;
    }

    for (const auto* imported : m_imports) {
        // cppcheck-suppress useStlAlgorithm
        if (auto* found = imported->find(name, recursive)) {
            return found;
        }
    }

    if (m_parent != nullptr) {
        if (auto* found = m_parent->find(name, recursive)) {
            return found;
        }
    }

    return nullptr;
}

std::vector<Symbol*> SymbolTable::getSymbols() const {
    std::vector<Symbol*> symbols;
    symbols.reserve(size());

    std::transform(m_symbols.begin(), m_symbols.end(), std::back_inserter(symbols), [](const auto& item) {
        return item.second;
    });

    std::sort(symbols.begin(), symbols.end(), [](auto lhs, auto rhs) {
        return lhs->getIndex() < rhs->getIndex();
    });

    return symbols;
}
