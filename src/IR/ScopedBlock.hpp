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

/**
 * A scoped block — a group of blocks sharing a lexical scope.
 *
 * ScopedBlock represents a lexical scope boundary in the IR. It contains
 * child blocks (which may themselves be scoped), an optional cleanup block
 * for explicit cleanup logic (e.g. retain/release), and a ValueTable for
 * named values declared within this scope. Any terminator that exits the
 * scope implicitly runs the cleanup block first. Destructors and deallocation
 * for types that require them are implicit at scope exit, derived from type
 * metadata.
 *
 * Three layers of cleanup:
 * 1. Explicit retain/release instructions within the body blocks.
 * 2. The cleanup block runs before scope exit (e.g. release operations).
 * 3. Implicit destructor/dealloc at scope boundary from type metadata.
 */
class ScopedBlock final : public Block {
public:
    ScopedBlock(Context& context, std::string label);

    /** Get the child blocks within this scope. */
    [[nodiscard]] auto blocks() -> llvm::ilist<Block>& { return m_blocks; }
    /** Get the cleanup instructions that run before scope exit. */
    [[nodiscard]] auto cleanup() -> llvm::ilist<Instruction>& { return m_cleanup; }
    /** Get the value table for named values declared in this scope. */
    [[nodiscard]] auto getValueTable() -> ValueTable& { return m_valueTable; }

    /** LLVM RTTI support. */
    [[nodiscard]] static constexpr auto classof(const Value* value) -> bool {
        return value->getKind() == Kind::ScopedBlock;
    }

private:
    ValueTable m_valueTable;            ///< named values in this scope
    llvm::ilist<Block> m_blocks;        ///< child blocks within the scope
    llvm::ilist<Instruction> m_cleanup; ///< cleanup instructions before scope exit
};

} // namespace lbc::ir
