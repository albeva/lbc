//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "pch.hpp"
#include "NamedValue.hpp"
#include "Symbol/SymbolTable.hpp"
namespace lbc::ir {

/** Maps names to IR NamedValues within a scope. */
class ValueTable final : public SymbolTableBase<NamedValue> {
public:
    using SymbolTableBase::SymbolTableBase;
};

} // namespace lbc::ir
