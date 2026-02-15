//
// Created by Albert Varaksin on 14/02/2026.
//
#pragma once
#include <utility>
#include "../GeneratorBase.hpp"

class AstGen;
class AstClass;

// -----------------------------------------------------------------------------
// Represent a member (args, custom functions, etc)
// -----------------------------------------------------------------------------

/**
 * Wraps a TableGen Member record. Determines whether the member is a
 * constructor parameter (no default value) or an initialized field, and
 * whether a setter should be generated (mutable bit).
 */
class AstMember final {
public:
    explicit AstMember(const Record* record);

    /// Whether this member generates a setter (mutable flag set in .td)
    [[nodiscard]] auto hasSetter() const -> bool { return m_mutable; }
    /// Whether this member is a constructor parameter (has no default value)
    [[nodiscard]] auto hasCtorParam() const -> bool { return m_ctorParam; }
    [[nodiscard]] auto getName() const -> const std::string& { return m_name; }
    [[nodiscard]] auto getType() const -> const std::string& { return m_type; }
    [[nodiscard]] auto getDefault() const -> const std::string& { return m_default; }
    /// Non-pointer types are passed as const in constructor and setter parameters
    [[nodiscard]] auto passAsConst() const -> bool {
        return m_type.back() != '*';
    }

private:
    std::string m_name;
    std::string m_type;
    std::string m_default;
    bool m_mutable;
    bool m_ctorParam;
};

// -----------------------------------------------------------------------------
// Represent AST class
// -----------------------------------------------------------------------------

/**
 * Represents a node in the AST class hierarchy. Built recursively from the
 * TableGen records — Root has no parent, Groups have children, Leaves are
 * concrete final classes. Generates C++ code fragments: constructor parameters,
 * initializer lists, data members, and accessor functions.
 */
class AstClass final {
public:
    enum class Kind : std::uint8_t {
        Root,
        Group,
        Leaf
    };

    AstClass(AstClass* parent, const AstGen& gen, const Record* record);

    [[nodiscard]] auto getParent() const -> AstClass* { return m_parent; }
    [[nodiscard]] auto getChildren() const -> const std::vector<std::unique_ptr<AstClass>>& { return m_children; }
    [[nodiscard]] auto getClassName() const -> const std::string& { return m_className; }
    [[nodiscard]] auto getEnumName() const -> const std::string& { return m_enumName; }
    [[nodiscard]] auto getRecord() const -> const Record* { return m_record; }
    [[nodiscard]] auto getKind() const -> Kind {
        if (m_parent == nullptr) {
            return Kind::Root;
        }
        return m_children.empty() ? Kind::Leaf : Kind::Group;
    }
    [[nodiscard]] auto isRoot() const -> bool { return getKind() == Kind::Root; }
    [[nodiscard]] auto isGroup() const -> bool { return getKind() == Kind::Group; }
    [[nodiscard]] auto isLeaf() const -> bool { return getKind() == Kind::Leaf; }

    /** Collect constructor parameter strings, recursing through parent chain. */
    [[nodiscard]] auto ctorParams() const -> std::vector<std::string>;
    /** Generate constructor initializer list entries (base class delegation + own members). */
    [[nodiscard]] auto ctorInitParams() const -> std::vector<std::string>;
    /** Generate private data member declarations. */
    [[nodiscard]] auto dataMembers() const -> std::vector<std::string>;
    /** Generate getter (and setter for mutable) function strings. */
    [[nodiscard]] auto functions() const -> std::vector<std::string>;
    /** Whether this class introduces any new constructor parameters beyond its parent. */
    [[nodiscard]] auto hasOwnCtorParams() -> bool;
    [[nodiscard]] auto getMembers() const -> const std::vector<std::unique_ptr<AstMember>>& { return m_members; }

private:
    AstClass* m_parent;
    const Record* m_record;
    /// C++ class name (e.g., "AstModule")
    std::string m_className;
    /// AstKind enum name (e.g., "Module")
    std::string m_enumName;
    std::vector<std::unique_ptr<AstClass>> m_children;
    std::vector<std::unique_ptr<AstMember>> m_members;
};

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
    void forwardDecls();
    void classGroup(AstClass* cls);
    void makeClass(AstClass* cls);
    void constructor(AstClass* cls);
    void functions(AstClass* cls);
    void classMembers(AstClass* cls);

    std::vector<const Record*> m_nodes;
    std::vector<const Record*> m_leaves;
    std::vector<const Record*> m_groups;
    const Record* m_nodeClass;
    const Record* m_leafClass;
    const Record* m_groupClass;
    /// Maps each Group record to its direct children (both Groups and Leaves)
    std::unordered_map<const Record*, std::vector<const Record*>> m_map;
};
