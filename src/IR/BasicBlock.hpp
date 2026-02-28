//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Block.hpp"
#include "Instruction.hpp"
namespace lbc {
class Context;
}
namespace lbc::ir {

/**
 * A basic block — a labeled, straight-line sequence of instructions.
 *
 * The last instruction in the body is a terminator (branch, conditional
 * branch, or return). BasicBlocks contain no scope or cleanup information;
 * for scoped lifetime management, use ScopedBlock.
 */
class BasicBlock final : public Block {
public:
    BasicBlock(Context& context, std::string label);

    /** Get the instruction list for this block. */
    [[nodiscard]] auto body() -> llvm::ilist<Instruction>& { return m_body; }

    /** LLVM RTTI support. */
    [[nodiscard]] static constexpr auto classof(const Value* value) -> bool {
        return value->getKind() == Kind::BasicBlock;
    }

private:
    llvm::ilist<Instruction> m_body; ///< instructions in this block
};

} // namespace lbc::ir
