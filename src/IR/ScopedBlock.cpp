//
// Created by Albert Varaksin on 28/02/2026.
//
#include "ScopedBlock.hpp"
using namespace lbc::ir;

ScopedBlock::ScopedBlock(Context& context, std::string label)
: Block(Kind::ScopedBlock, context, std::move(label))
, m_valueTable(nullptr) {}
