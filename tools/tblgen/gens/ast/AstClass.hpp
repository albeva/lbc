//
// Created by Albert Varaksin on 15/02/2026.
//
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "AstMember.hpp"

class AstGen;

namespace llvm {
class Record;
} // namespace llvm

/**
 * Represents a node in the AST class hierarchy. Built recursively from the
 * TableGen records -- Root has no parent, Groups have children, Leaves are
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

    AstClass(AstClass* parent, const AstGen& gen, const llvm::Record* record);

    [[nodiscard]] auto getParent() const -> AstClass* { return m_parent; }
    [[nodiscard]] auto getChildren() const -> const std::vector<std::unique_ptr<AstClass>>& { return m_children; }
    [[nodiscard]] auto getClassName() const -> const std::string& { return m_className; }
    [[nodiscard]] auto getEnumName() const -> const std::string& { return m_enumName; }
    [[nodiscard]] auto getRecord() const -> const llvm::Record* { return m_record; }
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
    const llvm::Record* m_record;
    /// C++ class name (e.g., "AstModule")
    std::string m_className;
    /// AstKind enum name (e.g., "Module")
    std::string m_enumName;
    std::vector<std::unique_ptr<AstClass>> m_children;
    std::vector<std::unique_ptr<AstMember>> m_members;
};
