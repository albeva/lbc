//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "pch.hpp"
#include "BasicBlock.hpp"
#include "Operand.hpp"
namespace lbc {
class Symbol;
class TypeFunction;
} // namespace lbc
namespace lbc::ir {

class Function : public Operand, public llvm::ilist_node<Function> {
public:
    Function(Symbol* symbol, std::string name);

    [[nodiscard]] static constexpr auto classof(const Value* value) -> bool {
        return value->getKind() == Kind::Function;
    }

    [[nodiscard]] auto blocks() noexcept -> llvm::ilist<BasicBlock>& { return m_blocks; }

    [[nodiscard]] auto getSymbol() const -> Symbol* { return m_symbol; }

private:
    Symbol* m_symbol;
    llvm::ilist<BasicBlock> m_blocks;
};

} // namespace lbc::ir
