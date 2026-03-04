//
// Created by Albert Varaksin on 01/03/2026.
//
#pragma once
#include <cstdint>
#include <functional>
#include <llvm/ADT/StringRef.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "TreeNodeArg.hpp"

namespace llvm {
class Record;
} // namespace llvm
namespace lib {
class TreeGenBase;

/**
 * Represents a node in the class hierarchy. Built recursively from
 * TableGen records -- Root has no parent, Groups have children, Leaves are
 * concrete final classes. Generates C++ code fragments: constructor parameters,
 * initializer lists, data members, and accessor functions.
 */
class TreeNode {
public:
    enum class Kind : std::uint8_t {
        Root,
        Group,
        Leaf
    };

    TreeNode(TreeNode* parent, const TreeGenBase& ctx, const llvm::Record* record);
    virtual ~TreeNode() = default;

    [[nodiscard]] auto getParent() const -> TreeNode* { return m_parent; }
    [[nodiscard]] auto getChildren() const -> const std::vector<std::unique_ptr<TreeNode>>& { return m_children; }
    [[nodiscard]] virtual auto getClassName() const -> std::string { return m_className; }
    [[nodiscard]] auto getEnumName() const -> const std::string& { return m_enumName; }
    [[nodiscard]] auto getRecord() const -> const llvm::Record* { return m_record; }
    [[nodiscard]] auto getKind() const -> Kind { return m_kind; }
    [[nodiscard]] auto isRoot() const -> bool { return getKind() == Kind::Root; }
    [[nodiscard]] auto isGroup() const -> bool { return getKind() == Kind::Group; }
    [[nodiscard]] auto isLeaf() const -> bool { return getKind() == Kind::Leaf; }

    /** Collect constructor parameter strings, recursing through parent chain. */
    [[nodiscard]] auto ctorParams() const -> std::vector<std::string>;
    [[nodiscard]] auto ctorArgs() const -> std::vector<std::string>;
    /** Generate constructor initializer list entries (base class delegation + own members). */
    [[nodiscard]] auto ctorInitParams() const -> std::vector<std::string>;
    /** Generate private data member declarations. */
    [[nodiscard]] auto classArgs() const -> std::vector<std::string>;
    /** Generate getter (and setter for mutable) function strings. */
    [[nodiscard]] auto classFunctions() const -> std::vector<std::string>;
    /** Whether this class introduces any new constructor parameters beyond its parent. */
    [[nodiscard]] auto hasOwnCtorParams() const -> bool;
    [[nodiscard]] auto getArgs() const -> const std::vector<std::unique_ptr<TreeNodeArg>>& { return m_args; }
    [[nodiscard]] auto getVisitorName() const -> std::string;

    [[nodiscard]] virtual auto getBaseClassName() const -> std::string;

    /** Find first and last leaf nodes in this subtree. */
    [[nodiscard]] auto getLeafRange() const -> std::optional<std::pair<const TreeNode*, const TreeNode*>>;

    template<std::invocable<const TreeNode*> Fn>
    void visit(Kind kind, Fn&& fn) const {
        if (getKind() == kind) {
            std::invoke(std::forward<Fn>(fn), this);
        }
        for (const auto& child : m_children) {
            child->visit(kind, std::forward<Fn>(fn));
        }
    }

    template<std::invocable<const TreeNode*> Fn>
    void visit(Fn&& fn) const {
        std::invoke(std::forward<Fn>(fn), this);
        for (const auto& child : m_children) {
            child->visit(std::forward<Fn>(fn));
        }
    }

private:
    [[nodiscard]] static auto unindent(llvm::StringRef code) -> std::string;

    TreeNode* m_parent;
    const llvm::Record* m_record;
    std::string m_prefix;
    /// C++ class name (e.g., "AstModule", "IrStore")
    std::string m_className;
    /// Kind enum name (e.g., "Module", "Store")
    std::string m_enumName;
    std::vector<std::unique_ptr<TreeNode>> m_children;
    std::vector<std::unique_ptr<TreeNodeArg>> m_args;
    std::vector<std::string> m_functions;
    Kind m_kind;
};
} // namespace lib
