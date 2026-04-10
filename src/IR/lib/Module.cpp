//
// Created by Albert Varaksin on 28/02/2026.
//
#include "Module.hpp"
#include "BasicBlock.hpp"
#include "Driver/Context.hpp"

using namespace lbc::ir::lib;

Module::Module(Context& context)
: m_globalInitBlock(context.create<BasicBlock>(context, "global.init")) {
}
