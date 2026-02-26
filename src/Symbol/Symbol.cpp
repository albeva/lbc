//
// Created by Albert Varaksin on 20/02/2026.
//
#include "Symbol.hpp"
#include "Type/Type.hpp"
using namespace lbc;

Symbol::Symbol(llvm::StringRef name, const Type* type, llvm::SMRange origin)
: m_name(name)
, m_type(type)
, m_range(origin)
, m_visibility(SymbolVisibility::Private) { }
