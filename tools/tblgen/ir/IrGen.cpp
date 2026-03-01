// Custom TableGen backend for generating IR instruction definitions.
// Reads Instructions.td and emits Instructions.hpp
#include "IrGen.hpp"
using namespace ir;
using namespace std::string_literals;

auto IrNodeClass::getClassName() const -> std::string {
    switch (getKind()) {
    case Kind::Root:
        return "Instruction";
    case Kind::Group:
        return "Ir" + getEnumName();
    case Kind::Leaf:
        return getEnumName() + "Instr";
    }
    std::unreachable();
}

auto IrNodeClass::getBaseClassName() const -> std::string {
    if (isRoot()) {
        return "llvm::ilist_node<" + getClassName() + ">";
    }
    return TreeNode::getBaseClassName();
}

IrGen::IrGen(
    raw_ostream& os,
    const RecordKeeper& records
)
: TreeGen(os, records, genName, "Ir", "lbc::ir") {}

auto IrGen::run() -> bool {
    forwardDecls();
    newline();

    treeKindEnum();
    newline();

    treeGroups(getRoot());
    return false;
}

void IrGen::forwardDecls() {
    line("class Block");
    line("class NamedValue");
    line("class Type");
    line("class Value");
}

void IrGen::treeNodeMethods(const lib::TreeNode* node) {
    TreeGen::treeNodeMethods(node);
    if (not node->isRoot()) {
        return;
    }

    comment("Get instruction mnemonic");
    block("[[nodiscard]] constexpr auto getMnemonic() const -> llvm::StringRef", [&] {
        line("const auto index = static_cast<std::size_t>(m_kind)");
        line("return kMnemonics.at(index)");
    });
    newline();
}

void IrGen::treeNodeData(const lib::TreeNode* node) {
    TreeGen::treeNodeData(node);
    if (not node->isRoot()) {
        return;
    }

    std::vector<std::string> mnemonics;
    getRoot()->visit(lib::TreeNode::Kind::Leaf, [&](const lib::TreeNode* leaf) {
        mnemonics.push_back(leaf->getRecord()->getValueAsString("mnemonic").str());
    });
    block("static constexpr std::array<llvm::StringRef, NODE_COUNT> kMnemonics", true, [&] {
        list(mnemonics, { .suffix = ",", .quote = true });
    });
}
