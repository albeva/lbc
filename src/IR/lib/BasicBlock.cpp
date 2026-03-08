//
// Created by Albert Varaksin on 28/02/2026.
//
#include "BasicBlock.hpp"
using namespace lbc::ir::lib;

BasicBlock::BasicBlock(Context& context, std::string label)
: Block(Kind::BasicBlock, context, std::move(label)) {}
