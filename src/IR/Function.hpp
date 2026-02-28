//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Block.hpp"
#include "NamedValue.hpp"
namespace lbc {
class Context;
class Symbol;
class TypeFunction;
} // namespace lbc
namespace lbc::ir {

class Function : public NamedValue, public llvm::ilist_node<Function> {
public:
    Function(Context& context, Symbol* symbol, std::string name);

    [[nodiscard]] static auto classof(const Value* value) -> bool {
        return value->getKind() == Kind::Function;
    }

    [[nodiscard]] auto blocks() noexcept -> llvm::ilist<Block>& { return m_blocks; }

    [[nodiscard]] auto getSymbol() const -> Symbol* { return m_symbol; }

private:
    Symbol* m_symbol;
    llvm::ilist<Block> m_blocks;
};

} // namespace lbc::ir
