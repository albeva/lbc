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

class BasicBlock final : public Block {
public:
    BasicBlock(Context& context, std::string label);

    [[nodiscard]] auto body() -> llvm::ilist<Instruction>& { return m_body; }

    [[nodiscard]] static constexpr auto classof(const Value* value) -> bool {
        return value->getKind() == Kind::BasicBlock;
    }

private:
    llvm::ilist<Instruction> m_body;
};

} // namespace lbc::ir
