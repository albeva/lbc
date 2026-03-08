//
// Created by Albert Varaksin on 28/02/2026.
//
#include "Function.hpp"
#include "Driver/Context.hpp"
#include "Symbol/Symbol.hpp"
#include "Type/Aggregate.hpp"
using namespace lbc::ir::lib;

Function::Function([[maybe_unused]] Context& /*context*/, Symbol* symbol)
: NamedValue(Kind::Function, symbol->getSymbolName().str(), symbol->getType())
, m_symbol(symbol) {}
