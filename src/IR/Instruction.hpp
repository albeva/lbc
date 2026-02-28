//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "pch.hpp"
namespace lbc::ir {

/**
 * Base class for IR instructions.
 *
 * Placeholder — concrete instruction types will be defined via TableGen
 * and generated into instruction classes with opcodes, operands, and
 * visitor dispatch.
 */
class Instruction : public llvm::ilist_node<Instruction> {
};

} // namespace lbc::ir
