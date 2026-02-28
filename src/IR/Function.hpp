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

/**
 * An IR function.
 *
 * Holds the function's name, type (via Symbol), and a list of blocks that
 * form the function body. The block list may contain both BasicBlocks and
 * ScopedBlocks. The first block is the entry point.
 */
class Function : public NamedValue, public llvm::ilist_node<Function> {
public:
    Function(Context& context, Symbol* symbol, std::string name);

    /** LLVM RTTI support. */
    [[nodiscard]] static auto classof(const Value* value) -> bool {
        return value->getKind() == Kind::Function;
    }

    /** Get the block list forming the function body. */
    [[nodiscard]] auto blocks() noexcept -> llvm::ilist<Block>& { return m_blocks; }

    /** Get the frontend symbol associated with this function. */
    [[nodiscard]] auto getSymbol() const -> Symbol* { return m_symbol; }

private:
    Symbol* m_symbol;            ///< frontend symbol with type and linkage info
    llvm::ilist<Block> m_blocks; ///< blocks forming the function body
};

} // namespace lbc::ir
