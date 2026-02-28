//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Block.hpp"
#include "Instruction.hpp"
#include "ValueTable.hpp"
namespace lbc {
class Context;
}
namespace lbc::ir {

class ScopedBlock final : public Block {
public:
    ScopedBlock(Context& context, std::string label);

    [[nodiscard]] auto blocks() -> llvm::ilist<Block>& { return m_blocks; }
    [[nodiscard]] auto cleanup() -> llvm::ilist<Instruction>& { return m_cleanup; }
    [[nodiscard]] auto getValueTable() -> ValueTable& { return m_valueTable; }

    [[nodiscard]] static constexpr auto classof(const Value* value) -> bool {
        return value->getKind() == Kind::ScopedBlock;
    }

private:
    ValueTable m_valueTable;
    llvm::ilist<Block> m_blocks;
    llvm::ilist<Instruction> m_cleanup;
};

} // namespace lbc::ir
