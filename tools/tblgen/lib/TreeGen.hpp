//
// Created by Albert Varaksin on 01/03/2026.
//
#pragma once
#include <concepts>
#include "GeneratorBase.hpp"
#include "TreeNode.hpp"
namespace lib {
class TreeGenBase : public GeneratorBase {
public:
    TreeGenBase(
        raw_ostream& os,
        const RecordKeeper& records,
        const StringRef generator,
        const StringRef prefix,
        const StringRef ns = "lbc",
        std::vector<StringRef> includes = { "pch.hpp" }
    )
    : GeneratorBase(os, records, generator, ns, std::move(includes))
    , m_prefix(prefix)
    , m_nodeRecords(sortedByDef(records.getAllDerivedDefinitions("Node")))
    , m_nodeClass(records.getClass("Node"))
    , m_leafClass(records.getClass("Leaf"))
    , m_groupClass(records.getClass("Group"))
    , m_argClass(records.getClass("Arg"))
    , m_funcClass(records.getClass("Func")) {}

    [[nodiscard]] auto getPrefix() const -> StringRef { return m_prefix; }
    [[nodiscard]] auto getNodeRecords() const -> const std::vector<const Record*>& { return m_nodeRecords; }
    [[nodiscard]] auto getNodeClass() const -> const Record* { return m_nodeClass; }
    [[nodiscard]] auto getLeafClass() const -> const Record* { return m_leafClass; }
    [[nodiscard]] auto getGroupClass() const -> const Record* { return m_groupClass; }
    [[nodiscard]] auto getArgClass() const -> const Record* { return m_argClass; }
    [[nodiscard]] auto getFuncClass() const -> const Record* { return m_funcClass; }
    [[nodiscard]] auto getRoot() const -> const TreeNode* { return m_root.get(); }

    virtual auto makeNode(TreeNode* parent, const Record* record) const -> std::unique_ptr<TreeNode> = 0;
    virtual auto makeArg(const Record* record) const -> std::unique_ptr<TreeNodeArg> = 0;

protected:
    void setRoot(std::unique_ptr<TreeNode> root) { m_root = std::move(root); }

private:
    StringRef m_prefix;
    std::vector<const Record*> m_nodeRecords;
    const Record* m_nodeClass;
    const Record* m_leafClass;
    const Record* m_groupClass;
    const Record* m_argClass;
    const Record* m_funcClass;
    std::unique_ptr<TreeNode> m_root;
};

/**
 * TableGen backend base class that loads a structured Node/Group/Leaf
 * tree from RecordKeeper. Templated on Class and Arg types.
 */
template<std::derived_from<TreeNode> ClassT, std::derived_from<TreeNodeArg> ArgT>
class TreeGen : public TreeGenBase {
public:
    TreeGen(
        raw_ostream& os,
        const RecordKeeper& records,
        const StringRef generator,
        const StringRef prefix,
        const StringRef ns = "lbc",
        std::vector<StringRef> includes = { "pch.hpp" }
    )
    : TreeGenBase(os, records, generator, prefix, ns, includes) {
        setRoot(TreeGen::makeNode(nullptr, records.getDef("Root")));
    }

    [[nodiscard]] auto makeNode(TreeNode* parent, const Record* record) const -> std::unique_ptr<TreeNode> override {
        return std::make_unique<ClassT>(parent, *this, record);
    }

    [[nodiscard]] auto makeArg(const Record* record) const -> std::unique_ptr<TreeNodeArg> override {
        return std::make_unique<ArgT>(record);
    }
};
} // namespace lib
