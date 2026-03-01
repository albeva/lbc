// Custom TableGen backend for generating AST node definitions.
// Reads Ast.td and emits Ast.hpp
#include "AstGen.hpp"
using namespace std::string_literals;
using namespace ast;

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
: NodeGen(os, records, generator, "Ast", ns, std::move(includes)) {}

auto AstGen::run() -> bool {
    forwardDecls();
    astNodesEnum();
    astForwardDecls();
    astGroup(getRoot());
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
        getRoot()->visit(lib::NodeClass::Kind::Leaf, [&](const auto* node) {
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
    getRoot()->visit([&](const auto* node) {
        line("class " + node->getClassName());
    });
    newline();
}

/**
 * Generate given class and all child classes.
 */
void AstGen::astGroup(const lib::NodeClass* cls) {
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
void AstGen::astClass(const lib::NodeClass* cls) {
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
void AstGen::constructor(const lib::NodeClass* cls) {
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
void AstGen::classof(const lib::NodeClass* cls) {
    scope(Scope::Public);

    const auto range = cls->getLeafRange();

    if (cls->isRoot()) {
        Builder::classof(getRoot()->getClassName(), "getKind", "");
    } else if (range) {
        if (range->first == range->second) {
            Builder::classof(getRoot()->getClassName(), "getKind", "AstKind", range->first->getEnumName());
        } else {
            Builder::classof(getRoot()->getClassName(), "getKind", "AstKind", range->first->getEnumName(), range->second->getEnumName());
        }
    }

    newline();
}

/**
 * Generate class methods
 */
void AstGen::functions(const lib::NodeClass* cls) {
    const auto functions = cls->classFunctions();
    if (functions.empty() && !cls->isRoot()) {
        return;
    }
    scope(Scope::Public);

    std::size_t count = 0;
    getRoot()->visit(lib::NodeClass::Kind::Leaf, [&](const lib::NodeClass* /* cls */) {
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
void AstGen::classArgs(const lib::NodeClass* cls) {
    const auto args = cls->classArgs();
    if (args.empty() && !cls->isRoot()) {
        return;
    }

    scope(Scope::Private);
    if (cls->isRoot()) {
        line("AstKind m_kind");
    }
    list(args, {});

    if (cls->isRoot()) {
        std::vector<std::string> classes;
        getRoot()->visit(lib::NodeClass::Kind::Leaf, [&](const lib::NodeClass* node) {
            classes.push_back(node->getClassName());
        });

        block("static constexpr std::array<llvm::StringRef, NODE_COUNT> kClassNames", true, [&] {
            list(classes, { .suffix = ",", .quote = true });
        });
    }
}
