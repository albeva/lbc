//
// Created by Albert Varaksin on 15/02/2026.
//
#include "AstArg.hpp"
#include <llvm/TableGen/Record.h>
using namespace llvm;

AstArg::AstArg(const Record* record)
: m_name(record->getValueAsString("name"))
, m_type(record->getValueAsString("type"))
, m_default(record->getValueAsString("default"))
, m_mutable(record->getValueAsBit("mutable"))
, m_ctorParam(m_default.empty()) { }
