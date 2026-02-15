//
// Created by Albert Varaksin on 14/02/2026.
//
#pragma once
#include <utility>
#include "../../GeneratorBase.hpp"
#include "AstClass.hpp"

// -----------------------------------------------------------------------------
// The generator
// -----------------------------------------------------------------------------

/**
 * TableGen backend that reads Ast.td and emits Ast.inc. Builds an in-memory
 * AstClass tree mirroring the Node/Group/Leaf hierarchy, then walks it to
 * generate: AstKind enum, forward declarations, and complete C++ class
 * definitions with constructors, accessors, and data members.
 */
class AstGen final : public GeneratorBase {
public:
    static constexpr auto genName = "lbc-ast-def";

    AstGen(
        raw_ostream& os,
        const RecordKeeper& records
    );

    [[nodiscard]] auto run() -> bool final;
    void forwardDecls();

    [[nodiscard]] auto getNodes() const -> std::vector<const Record*> { return m_nodes; }
    [[nodiscard]] auto getLeaves() const -> std::vector<const Record*> { return m_leaves; }
    [[nodiscard]] auto getGroups() const -> std::vector<const Record*> { return m_groups; }
    [[nodiscard]] auto getNodeClass() const -> const Record* { return m_nodeClass; }
    [[nodiscard]] auto getLeafClass() const -> const Record* { return m_leafClass; }
    [[nodiscard]] auto getGroupClass() const -> const Record* { return m_groupClass; }
    /** Parent group → child nodes mapping, used by AstClass to build the tree. */
    [[nodiscard]] auto getMap() const -> const std::unordered_map<const Record*, std::vector<const Record*>>& { return m_map; }

private:
    /// Root of the AstClass tree, built in the constructor
    std::unique_ptr<AstClass> m_root;

    void astNodesEnum();
    void astForwardDecls();
    void astGroup(AstClass* cls);
    void astClass(AstClass* cls);
    void constructor(AstClass* cls);
    void classof(AstClass* cls);
    void functions(AstClass* cls);
    void members(AstClass* cls);

    std::vector<const Record*> m_nodes;
    std::vector<const Record*> m_leaves;
    std::vector<const Record*> m_groups;
    const Record* m_nodeClass;
    const Record* m_leafClass;
    const Record* m_groupClass;
    /// Maps each Group record to its direct children (both Groups and Leaves)
    std::unordered_map<const Record*, std::vector<const Record*>> m_map;
    std::vector<std::string> m_classNames;
};
