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

class Block : public NamedValue, public llvm::ilist_node<Block> {
public:
    [[nodiscard]] static constexpr auto classof(const Value* value) -> bool {
        return value->getKind() >= Kind::BasicBlock && value->getKind() <= Kind::ScopedBlock;
    }

protected:
    Block(Kind kind, Context& context, std::string label);
};

} // namespace lbc::ir
