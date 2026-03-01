// Custom TableGen backend for generating IR instruction definitions.
// Reads Instructions.td and emits Instructions.hpp
#include "IrGen.hpp"
using namespace ir;
using namespace std::string_literals;

IrGen::IrGen(
    raw_ostream& os,
    const RecordKeeper& records
)
: GeneratorBase(os, records, genName, "lbc::ir", { "pch.hpp", "IR/Instruction.hpp" })
, m_root(nullptr)
, nodeRecords(sortedByDef(records.getAllDerivedDefinitions("Node")))
, m_nodeClass(records.getClass("Node"))
, m_leafClass(records.getClass("Leaf"))
, m_groupClass(records.getClass("Group"))
, m_argClass(records.getClass("Arg"))
, m_funcClass(records.getClass("Func")) {
    m_root = std::make_unique<IrClass>(nullptr, *this, records.getDef("Root"));
}

IrGen::~IrGen() = default;

auto IrGen::run() -> bool {
    forwardDecls();
    irNodesEnum();
    irGroup(m_root.get());
    return false;
}

void IrGen::forwardDecls() {
    line("class Block");
    line("class NamedValue");
    line("class Type");
    line("class Value");
}

void IrGen::irNodesEnum() {
    doc("Enumerates all concrete IR node kinds.\nValues are ordered by group for efficient range-based membership checks.");
    block("enum class IrKind : std::uint8_t", true, [&] {
        m_root->visit(IrClass::Kind::Leaf, [&](const auto* node) {
            line(node->getEnumName(), ",");
        });
    });
    newline();
}

/**
 * Generate given class and all child classes.
 */
void IrGen::irGroup(const IrClass* cls) {
    if (cls->isLeaf()) {
        irClass(cls);
    } else {
        section((cls->getRecord()->getName() + " nodes").str());
        irClass(cls);
        for (const auto& child : cls->getChildren()) {
            irGroup(child.get());
        }
    }
}

/**
 * Generate Ast class
 */
void IrGen::irClass(const IrClass* cls) {
    const auto base = cls->isRoot() ? " : public llvm::ilist_node<" + cls->getClassName() + ">" : " : public " + cls->getParent()->getClassName();
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
void IrGen::constructor(const IrClass* cls) {
    if (cls->isLeaf()) {
        scope(Scope::Public);
    } else {
        scope(Scope::Protected);
    }

    // constructor
    if (cls->isLeaf() || cls->isRoot() || cls->hasOwnCtorParams()) {
        const auto params = cls->ctorParams();
        const bool isExplicit = params.size() <= 1;
        doc("Construct "s + articulate(cls->getClassName()) + cls->getClassName() + " node");
        line("constexpr "s + (isExplicit ? "explicit " : "") + cls->getClassName() + "(", "");
        indent(false, [&] {
            if (cls->isRoot() || cls->isGroup()) {
                line("const IrKind kind", params.empty() ? "" : ",");
            }
            list(params, { .suffix = "," });
        });
        line(")", "");

        // constructor init
        if (cls->isRoot()) {
            line(": m_kind(kind)", " {}");
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
void IrGen::classof(const IrClass* cls) {
    scope(Scope::Public);

    const auto range = cls->getLeafRange();

    if (cls->isRoot()) {
        Builder::classof(m_root->getClassName(), "getKind", "");
    } else if (range) {
        if (range->first == range->second) {
            Builder::classof(m_root->getClassName(), "getKind", "IrKind", range->first->getEnumName());
        } else {
            Builder::classof(m_root->getClassName(), "getKind", "IrKind", range->first->getEnumName(), range->second->getEnumName());
        }
    }
}

/**
 * Generate class methods
 */
void IrGen::functions(const IrClass* cls) {
    const auto functions = cls->classFunctions();
    if (functions.empty() && !cls->isRoot()) {
        return;
    }
    newline();
    scope(Scope::Public);

    std::size_t count = 0;
    m_root->visit(IrClass::Kind::Leaf, [&](const IrClass* /* cls */) {
        count++;
    });

    if (cls->isRoot()) {
        comment("Number of AST leaf nodes");
        line("static constexpr std::size_t NODE_COUNT = " + std::to_string(count));
        newline();

        comment("Get the kind discriminator for this node");
        block("[[nodiscard]] constexpr auto getKind() const -> IrKind", [&] {
            line("return m_kind");
        });
        newline();

        comment("Get IR node class name");
        block("[[nodiscard]] constexpr auto getClassName() const -> llvm::StringRef", [&] {
            line("const auto index = static_cast<std::size_t>(m_kind)");
            line("return kClassNames.at(index)");
        });
        newline();

        comment("Get instruction mnemonic");
        block("[[nodiscard]] constexpr auto getMnemonic() const -> llvm::StringRef", [&] {
            line("const auto index = static_cast<std::size_t>(m_kind)");
            line("return kMnemonics.at(index)");
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
void IrGen::classArgs(const IrClass* cls) {
    const auto args = cls->classArgs();
    if (args.empty() && !cls->isRoot()) {
        return;
    }

    scope(Scope::Private);
    if (cls->isRoot()) {
        line("IrKind m_kind");
    }
    list(args, {});

    if (cls->isRoot()) {
        std::vector<std::string> classes;
        std::vector<std::string> mnemonics;
        m_root->visit(IrClass::Kind::Leaf, [&](const IrClass* node) {
            classes.push_back(node->getClassName());
            mnemonics.push_back(node->getRecord()->getValueAsString("mnemonic").str());
        });

        block("static constexpr std::array<llvm::StringRef, NODE_COUNT> kClassNames", true, [&] {
            list(classes, { .suffix = ",", .quote = true });
        });
        block("static constexpr std::array<llvm::StringRef, NODE_COUNT> kMnemonics", true, [&] {
            list(mnemonics, { .suffix = ",", .quote = true });
        });
    }
}
