//
// Created by Albert Varaksin on 01/03/2026.
//
#include "TreeNodeArg.hpp"
#include <llvm/TableGen/Record.h>
using namespace lib;

TreeNodeArg::TreeNodeArg(const llvm::Record* record)
: m_name(record->getValueAsString("name"))
, m_type(record->getValueAsString("type"))
, m_default(record->getValueAsString("default"))
, m_mutable(record->getValueAsBit("mutable"))
, m_ctorParam(m_default.empty()) {}
