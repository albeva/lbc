//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "pch.hpp"
#include "NamedValue.hpp"
namespace lbc {
class Context;
}
namespace lbc::ir {

/**
 * Abstract base for IR blocks.
 *
 * A Block is a labeled unit within a function's control flow graph.
 * Concrete subclasses are BasicBlock (a flat sequence of instructions)
 * and ScopedBlock (a group of blocks sharing a lexical scope with an
 * optional cleanup block). Blocks use the Label sentinel type.
 */
class Block : public NamedValue, public llvm::ilist_node<Block> {
public:
    /** LLVM RTTI support. */
    [[nodiscard]] static constexpr auto classof(const Value* value) -> bool {
        return value->getKind() >= Kind::BasicBlock && value->getKind() <= Kind::ScopedBlock;
    }

protected:
    Block(Kind kind, Context& context, std::string label);
};

} // namespace lbc::ir
