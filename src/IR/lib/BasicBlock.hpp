//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Instructions.hpp"
#include "NamedValue.hpp"
namespace lbc {
class Context;
}
namespace lbc::ir::lib {

/**
 * A Block is a labeled unit within a function's control flow graph.
 */
class BasicBlock : public NamedValue, public llvm::ilist_node<BasicBlock> {
public:
    BasicBlock(Context& context, std::string label);

    /** Get the instruction list for this block. */
    [[nodiscard]] auto getBody() -> llvm::ilist<Instruction>& { return m_body; }
    [[nodiscard]] auto getBody() const -> const llvm::ilist<Instruction>& { return m_body; }

    /** LLVM RTTI support. */
    [[nodiscard]] static auto classof(const Value* value) -> bool {
        return value->getKind() == Kind::BasicBlock;
    }

private:
    llvm::ilist<Instruction> m_body; ///< instructions in this block
};

} // namespace lbc::ir::lib
