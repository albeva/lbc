//
// Created by Albert Varaksin on 28/02/2026.
//
#include "ScopedBlock.hpp"
using namespace lbc::ir;

ScopedBlock::ScopedBlock(Context& context, std::string label, ScopedBlock* parent)
: Block(Kind::ScopedBlock, context, std::move(label))
, m_parent(parent)
, m_valueTable(parent != nullptr ? parent->getValueTable() : nullptr) {}
