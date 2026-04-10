//
// Created by Albert Varaksin on 28/02/2026.
//
#include "BasicBlock.hpp"
#include "Driver/Context.hpp"
#include "Type/Type.hpp"
#include "Type/TypeFactory.hpp"
using namespace lbc::ir::lib;

BasicBlock::BasicBlock(Context& context, std::string label)
: NamedValue(Kind::BasicBlock, std::move(label), context.getTypeFactory().getLabel()) {}
