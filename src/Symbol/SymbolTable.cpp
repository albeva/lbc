//
// Created by Albert Varaksin on 06/07/2020.
//
#include "SymbolTable.hpp"

#include "Driver/Context.hpp"
#include "Symbol.hpp"
#include <algorithm>
using namespace lbc;

void SymbolTable::insert(Symbol* symbol) {
    m_symbols.insert({ symbol->name(), symbol });
}

void SymbolTable::import(SymbolTable* table) {
    m_imports.push_back(table);
}

auto SymbolTable::find(llvm::StringRef name, bool recursive) const -> Symbol* {
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

auto SymbolTable::getSymbols() const -> std::vector<Symbol*> {
    std::vector<Symbol*> symbols;
    symbols.reserve(size());

    std::ranges::transform(m_symbols, std::back_inserter(symbols), [](const auto& item) {
        return item.second;
    });

    std::ranges::sort(symbols, [](auto lhs, auto rhs) {
        return lhs->getIndex() < rhs->getIndex();
    });

    return symbols;
}
