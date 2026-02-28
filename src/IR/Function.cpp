//
// Created by Albert Varaksin on 28/02/2026.
//
#include "Function.hpp"
#include "Driver/Context.hpp"
#include "Symbol/Symbol.hpp"
#include "Type/Aggregate.hpp"
using namespace lbc::ir;

Function::Function([[maybe_unused]] Context& /*context*/, Symbol* symbol, std::string name)
: NamedValue(Kind::Function, std::move(name), symbol->getType())
, m_symbol(symbol) {}
