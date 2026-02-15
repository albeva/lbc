//
// Created by Albert Varaksin on 15/02/2026.
//
#pragma once
#include <cstdint>
#include <functional>
#include <llvm/ADT/StringRef.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "AstArg.hpp"

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
    [[nodiscard]] auto getKind() const -> Kind { return m_kind; }
    [[nodiscard]] auto isRoot() const -> bool { return getKind() == Kind::Root; }
    [[nodiscard]] auto isGroup() const -> bool { return getKind() == Kind::Group; }
    [[nodiscard]] auto isLeaf() const -> bool { return getKind() == Kind::Leaf; }

    /** Collect constructor parameter strings, recursing through parent chain. */
    [[nodiscard]] auto ctorParams() const -> std::vector<std::string>;
    /** Generate constructor initializer list entries (base class delegation + own members). */
    [[nodiscard]] auto ctorInitParams() const -> std::vector<std::string>;
    /** Generate private data member declarations. */
    [[nodiscard]] auto classArgs() const -> std::vector<std::string>;
    /** Generate getter (and setter for mutable) function strings. */
    [[nodiscard]] auto classFunctions() const -> std::vector<std::string>;
    /** Whether this class introduces any new constructor parameters beyond its parent. */
    [[nodiscard]] auto hasOwnCtorParams() -> bool;
    [[nodiscard]] auto getArgs() const -> const std::vector<std::unique_ptr<AstArg>>& { return m_args; }
    [[nodiscard]] auto getVisitorName() const -> std::string;

    /** Find first and last child items. if nested is true, then recursievly find among sub childs */
    [[nodiscard]] auto getLeafRange() const -> std::optional<std::pair<const AstClass*, const AstClass*>>;

    template <std::invocable<const AstClass*> Fn>
    void visit(Kind kind, Fn&& fn) const {
        if (getKind() == kind) {
            std::invoke(std::forward<Fn>(fn), this);
        }
        for (const auto& child : m_children) {
            child->visit(kind, std::forward<Fn>(fn));
        }
    }

    template <std::invocable<const AstClass*> Fn>
    void visit(Fn&& fn) const {
        std::invoke(std::forward<Fn>(fn), this);
        for (const auto& child : m_children) {
            child->visit(std::forward<Fn>(fn));
        }
    }

private:
    [[nodiscard]] static auto unindent(llvm::StringRef code) -> std::string;

    AstClass* m_parent;
    const llvm::Record* m_record;
    /// C++ class name (e.g., "AstModule")
    std::string m_className;
    /// AstKind enum name (e.g., "Module")
    std::string m_enumName;
    std::vector<std::unique_ptr<AstClass>> m_children;
    std::vector<std::unique_ptr<AstArg>> m_args;
    std::vector<std::string> m_functions;
    Kind m_kind;
};
