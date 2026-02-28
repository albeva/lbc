//
// Created by Albert Varaksin on 28/02/2026.
//
#include "Block.hpp"
#include "Driver/Context.hpp"
#include "Type/Type.hpp"
#include "Type/TypeFactory.hpp"
using namespace lbc::ir;

Block::Block(const Kind kind, Context& context, std::string label)
: NamedValue(kind, std::move(label), context.getTypeFactory().getLabel()) {}
