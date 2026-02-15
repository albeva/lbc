// Custom TableGen backend for generating AST node definitions.
// Reads Ast.td and emits Ast.inc
#include "AstGen.hpp"
using namespace std::string_literals;

// -----------------------------------------------------------------------------
// The generator
// -----------------------------------------------------------------------------

AstGen::AstGen(
    raw_ostream& os,
    const RecordKeeper& records
)
: GeneratorBase(os, records, genName, "lbc", { "\"pch.hpp\"", "\"Utilities/LiteralValue.hpp\"" })
, m_root(nullptr)
, m_nodeClass(records.getClass("Node"))
, m_leafClass(records.getClass("Leaf"))
, m_groupClass(records.getClass("Group")) {
    // nodes
    m_nodes = sortedByDef(records.getAllDerivedDefinitions("Node"));
    m_leaves = sortedByDef(records.getAllDerivedDefinitions("Leaf"));
    m_groups = sortedByDef(records.getAllDerivedDefinitions("Group"));

    // build up relations map
    for (const Record* group : m_groups) {
        auto children = collect(m_nodes, "parent", group);
        m_map.try_emplace(group, std::move(children));
    }

    // build class graph
    const auto* root = records.getDef("Root");
    m_root = std::make_unique<AstClass>(nullptr, *this, root);
}

auto AstGen::run() -> bool {
    forwardDecls();
    astNodesEnum();
    astForwardDecls();
    astGroup(m_root.get());
    return false;
}

/**
 * Generate forward declarations of types required by ast
 */
void AstGen::forwardDecls() {
    line("enum class TokenKind: std::uint8_t");
    line("class Type");
    newline();
}

/**
 * Generate AstKind enum type
 *
 * This finds enums recursievly to ensure that enums are defined
 * in exact order and grouped together, so we can use range checks
 * if a node belongs in a group.
 *
 * This also generates m_classNames, ensuring names are in matching order
 */
void AstGen::astNodesEnum() {
    m_classNames.reserve(m_leaves.size());

    doc("Enumerates all concrete AST node kinds.\nValues are ordered by group for efficient range-based membership checks.");
    block("enum class AstKind : std::uint8_t", true, [&] {
        const auto recurse = [&](this auto&& self, const AstClass* cls) -> void {
            if (cls->isLeaf()) {
                line(cls->getEnumName(), ",");
                m_classNames.emplace_back(cls->getClassName());
            } else {
                for (const auto& child : cls->getChildren()) {
                    self(child.get());
                }
            }
        };
        recurse(m_root.get());
    });
    newline();
}

/**
 * Emit ast class forward declarations
 */
void AstGen::astForwardDecls() {
    section("Forward Declarations");
    for (const Record* node : m_nodes) {
        line("class Ast" + node->getName());
    }
    newline();
}

/**
 * Generate given class and all child classes.
 */
void AstGen::astGroup(AstClass* cls) {
    if (cls->isLeaf()) {
        astClass(cls);
    } else {
        section((cls->getRecord()->getName() + " nodes").str());
        astClass(cls);
        for (const auto& child : cls->getChildren()) {
            astGroup(child.get());
        }
    }
}

/**
 * Generate Ast class
 */
void AstGen::astClass(AstClass* cls) {
    const auto base = cls->isRoot() ? "" : " : public " + cls->getParent()->getClassName();
    const auto* const final = cls->isLeaf() ? " final" : "";

    if (cls->isGroup()) {
        doc("Abstract base for all "s + cls->getRecord()->getValueAsString("desc").str() + " nodes");
    } else {
        doc(cls->getRecord()->getValueAsString("desc"));
    }
    block("class [[nodiscard]] " + cls->getClassName() + final + base, true, [&] {
        m_scope = Scope::Private;
        if (cls->isRoot()) {
            scope(Scope::Public);
            line("NO_COPY_AND_MOVE(" + cls->getClassName() + ")", "");
            newline();
        }

        constructor(cls);
        classof(cls);
        functions(cls);
        members(cls);
    });
    newline();
}

/**
 * Generate class constructor
 */
void AstGen::constructor(AstClass* cls) {
    if (cls->isLeaf()) {
        scope(Scope::Public);
    } else {
        scope(Scope::Protected);
    }

    // constructor
    if (cls->isLeaf() || cls->hasOwnCtorParams()) {
        const auto params = cls->ctorParams();
        const bool isExplicit = cls->isLeaf() && params.size() == 1;
        doc("Construct "s + articulate(cls->getClassName()) + cls->getClassName() + " node");
        line("constexpr "s + (isExplicit ? "explicit " : "") + cls->getClassName() + "(", "");
        indent(false, [&] {
            if (cls->isRoot() || cls->isGroup()) {
                line("const AstKind kind", ",");
            }
            list(params, { .suffix = "," });
        });
        line(")", "");

        // constructor init
        if (cls->isRoot()) {
            line(": m_kind(kind)", "");
        }
        list(
            cls->ctorInitParams(),
            { .firstPrefix = (cls->isRoot() ? ", " : ": "), .prefix = ", ", .lastSuffix = " {}" }
        );
    } else {
        line("using " + cls->getParent()->getClassName() + "::" + cls->getParent()->getClassName());
    }
    newline();
}

/**
 * Generate classof method for llvm rtti support
 */
void AstGen::classof(AstClass* cls) {
    scope(Scope::Public);
    comment("LLVM RTTI support to check if given node is "s + articulate(cls->getClassName()) + cls->getClassName());
    block("[[nodiscard]] static constexpr auto classof(const "s + m_root->getClassName() + "* " + (cls->isRoot() ? "/* ast */" : " ast") + ") -> bool", [&] {
        if (cls->isRoot()) {
            line("return true");
        } else if (cls->isLeaf()) {
            line("return ast->getKind() == AstKind::" + cls->getEnumName());
        } else {
            line("return ast->getKind() >= AstKind::" + cls->getChildren().front()->getEnumName() + " && ast->getKind() <= AstKind::" + cls->getChildren().back()->getEnumName());
        }
    });
    newline();
}

/**
 * Generate class methods
 */
void AstGen::functions(AstClass* cls) {
    const auto functions = cls->functions();
    if (functions.empty() && !cls->isRoot()) {
        return;
    }
    scope(Scope::Public);

    if (cls->isRoot()) {
        comment("Get the kind discriminator for this node");
        block("[[nodiscard]] constexpr auto getKind() const -> AstKind", [&] {
            line("return m_kind");
        });
        newline();

        comment("Get ast node class name");
        block("[[nodiscard]] constexpr auto getClassName() const -> llvm::StringRef", [&] {
            line("const auto index = static_cast<std::size_t>(m_kind)");
            line("return kClassNames.at(index)");
        });
        newline();
    }

    for (const auto& func : functions) {
        lines(func, "\n");
        newline();
    }
}

/**
 * Generate class data members
 */
void AstGen::members(AstClass* cls) {
    const auto members = cls->dataMembers();
    if (members.empty() && !cls->isRoot()) {
        return;
    }

    scope(Scope::Private);
    if (cls->isRoot()) {
        line("AstKind m_kind");
    }
    list(members, {});

    if (cls->isRoot()) {
        block("static constexpr std::array<llvm::StringRef, " + std::to_string(m_classNames.size()) + "> kClassNames", true, [&] {
            list(m_classNames, { .suffix = ",", .quote = true });
        });
    }
}
