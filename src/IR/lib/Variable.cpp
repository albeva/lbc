//
// Created by Albert Varaksin on 08/03/2026.
//
#include "Variable.hpp"

#include "Symbol/Symbol.hpp"
using namespace lbc::ir::lib;

Variable::Variable(Symbol* symbol)
: NamedValue(Kind::Variable, symbol->getSymbolName().str(), symbol->getType())
, m_symbol(symbol) {}
