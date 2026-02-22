// Custom TableGen backend for generating AST node definitions.
// Reads Ast.td and emits Ast.hpp
#include "AstGen.hpp"
using namespace std::string_literals;

// -----------------------------------------------------------------------------
// The generator
// -----------------------------------------------------------------------------

AstGen::AstGen(
    raw_ostream& os,
    const RecordKeeper& records,
    StringRef generator,
    StringRef ns,
    std::vector<StringRef> includes
)
: GeneratorBase(os, records, generator, ns, std::move(includes))
, m_root(nullptr)
, nodeRecords(sortedByDef(records.getAllDerivedDefinitions("Node")))
, m_nodeClass(records.getClass("Node"))
, m_leafClass(records.getClass("Leaf"))
, m_groupClass(records.getClass("Group"))
, m_argClass(records.getClass("Arg"))
, m_funcClass(records.getClass("Func")) {
    m_root = std::make_unique<AstClass>(nullptr, *this, records.getDef("Root"));
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
    line("class Type");
    line("class SymbolTable");
    line("class Symbol");
    newline();
}

/**
 * Generate AstKind enum type
 */
void AstGen::astNodesEnum() {
    doc("Enumerates all concrete AST node kinds.\nValues are ordered by group for efficient range-based membership checks.");
    block("enum class AstKind : std::uint8_t", true, [&] {
        m_root->visit(AstClass::Kind::Leaf, [&](const auto* node) {
            line(node->getEnumName(), ",");
        });
    });
    newline();
}

/**
 * Emit ast class forward declarations
 */
void AstGen::astForwardDecls() {
    section("Forward Declarations");
    m_root->visit([&](const auto* node) {
        line("class " + node->getClassName());
    });
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
        classArgs(cls);
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

    const auto range = cls->getLeafRange();
    const bool hideParam = cls->isRoot() || !range.has_value();

    comment("LLVM RTTI support to check if given node is "s + articulate(cls->getClassName()) + cls->getClassName());
    block("[[nodiscard]] static constexpr auto classof(const "s + m_root->getClassName() + "* " + (hideParam ? "/* ast */" : "ast") + ") -> bool", [&] {
        if (cls->isRoot()) {
            line("return true");
        } else if (range) {
            if (range->first == range->second) {
                line("return ast->getKind() == AstKind::" + range->first->getEnumName());
            } else {
                line("return ast->getKind() >= AstKind::" + range->first->getEnumName() + " && ast->getKind() <= AstKind::" + range->second->getEnumName());
            }
        } else {
            line("return false");
        }
    });
    newline();
}

/**
 * Generate class methods
 */
void AstGen::functions(AstClass* cls) {
    const auto functions = cls->classFunctions();
    if (functions.empty() && !cls->isRoot()) {
        return;
    }
    scope(Scope::Public);

    std::size_t count = 0;
    m_root->visit(AstClass::Kind::Leaf, [&](const AstClass* /* cls */) {
        count++;
    });

    if (cls->isRoot()) {
        comment("Number of AST leaf nodes");
        line("static constexpr std::size_t NODE_COUNT = " + std::to_string(count));
        newline();

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
 * Generate class data args
 */
void AstGen::classArgs(AstClass* cls) {
    const auto args = cls->classArgs();
    if (args.empty() && !cls->isRoot()) {
        return;
    }

    scope(Scope::Private);
    if (cls->isRoot()) {
        line("AstKind m_kind");
    }
    list(args, {});

    std::vector<std::string> m_classes;
    m_root->visit(AstClass::Kind::Leaf, [&](const AstClass* node) {
        m_classes.push_back(node->getClassName());
    });

    if (cls->isRoot()) {
        block("static constexpr std::array<llvm::StringRef, NODE_COUNT> kClassNames", true, [&] {
            list(m_classes, { .suffix = ",", .quote = true });
        });
    }
}
