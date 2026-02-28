//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "pch.hpp"
namespace lbc::ir {

class Instruction : public llvm::ilist_node<Instruction> {
};

} // namespace lbc::ir
