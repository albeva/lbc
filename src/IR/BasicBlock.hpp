//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Instruction.hpp"
#include "Operand.hpp"
namespace lbc::ir {

class BasicBlock final : public Operand, public llvm::ilist_node<BasicBlock> {
public:
    BasicBlock(bool scoped, std::string label);
    ~BasicBlock() override;

    [[nodiscard]] auto isScoped() const -> bool { return m_scoped; }
    [[nodiscard]] auto body() -> llvm::ilist<Instruction>& { return m_body; }
    [[nodiscard]] auto cleanup() -> llvm::ilist<Instruction>& { return m_cleanup; }

    [[nodiscard]] static constexpr auto classof(const Value* value) -> bool {
        return value->getKind() == Kind::BasicBlock;
    }

private:
    llvm::ilist<Instruction> m_body;
    llvm::ilist<Instruction> m_cleanup;
    bool m_scoped;
};

} // namespace lbc::ir
