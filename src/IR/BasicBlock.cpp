//
// Created by Albert Varaksin on 28/02/2026.
//
#include "BasicBlock.hpp"
using namespace lbc::ir;

BasicBlock::BasicBlock(const bool scoped, std::string label)
: Operand(Kind::BasicBlock, label, nullptr)
, m_scoped(scoped) {}

BasicBlock::~BasicBlock() = default;
