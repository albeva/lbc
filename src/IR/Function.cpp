//
// Created by Albert Varaksin on 28/02/2026.
//
#include "Function.hpp"
#include "Symbol/Symbol.hpp"
#include "Type/Aggregate.hpp"
using namespace lbc::ir;

Function::Function(Symbol* symbol, std::string name)
: Operand(Kind::Function, std::move(name), symbol->getType())
, m_symbol(symbol) {}
