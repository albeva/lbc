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
    [[nodiscard]] auto getClassName() const -> std::string override;
    [[nodiscard]] auto getBaseClassName() const -> std::string override;
};

/**
 * TableGen backend that reads Instructions.td and emits Instructions.hpp.
 */
class IrGen final : public lib::TreeGen<IrNodeClass> {
public:
    static constexpr auto genName = "lbc-ir-inst-def";

    IrGen(raw_ostream& os, const RecordKeeper& records);

    [[nodiscard]] auto run() -> bool override;

    void forwardDecls();

protected:
    void treeNodeMethods(const lib::TreeNode* node) override;
    void treeNodeData(const lib::TreeNode* node) override;
};
} // namespace ir
