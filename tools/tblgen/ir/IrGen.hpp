//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include "lib/TreeGen.hpp"
namespace ir {
using namespace llvm;

class IrNodeClass final : public lib::TreeNode {
public:
    using TreeNode::TreeNode;
};

/** TableGen backend that reads Instructions.td and emits Instructions.hpp. */
class IrGen final : public lib::TreeGen<IrNodeClass, lib::TreeNodeArg> {
public:
    static constexpr auto genName = "lbc-ir-inst-def";

    IrGen(raw_ostream& os, const RecordKeeper& records);

    [[nodiscard]] auto run() -> bool override;

    void forwardDecls();
    void irNodesEnum();
    void irGroup(const lib::TreeNode* cls);
    void irClass(const lib::TreeNode* cls);
    void constructor(const lib::TreeNode* cls);
    void classof(const lib::TreeNode* cls);
    void functions(const lib::TreeNode* cls);
    void classArgs(const lib::TreeNode* cls);
};
} // namespace ir
