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
    // forward declare
    line("enum class TokenKind: std::uint8_t");
    line("class Type").newline();

    astNodesEnum();
    forwardDecls();
    classGroup(m_root.get());
    return false;
}

void AstGen::astNodesEnum() {
    // This finds enums recursievly to ensure that enums are defined
    // in exact order and grouped together, so we can use range checks
    // if a node belongs in a group.
    doc("Enumrate all ast leaf nodes");
    block("enum class AstKind : std::uint8_t", true, [&] {
        const auto recurse = [&](this auto&& self, const AstClass* cls) -> void {
            if (cls->isLeaf()) {
                line(cls->getEnumName(), ",");
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

void AstGen::forwardDecls() {
    section("Forward Declarations");
    for (const Record* node : m_nodes) {
        line("class Ast" + node->getName());
    }
    newline();
}

void AstGen::classGroup(AstClass* cls) {
    switch (cls->getKind()) {
    case AstClass::Kind::Root:
    case AstClass::Kind::Group:
        section((cls->getRecord()->getName() + " nodes").str());
        makeClass(cls);
        for (const auto& child : cls->getChildren()) {
            classGroup(child.get());
        }
        return;
    case AstClass::Kind::Leaf:
        makeClass(cls);
        return;
    }
}

void AstGen::makeClass(AstClass* cls) {
    const auto base = cls->isRoot() ? "" : " : public " + cls->getParent()->getClassName();
    const auto* const final = cls->isLeaf() ? " final" : "";

    doc(cls->getRecord()->getValueAsString("desc"));
    block("class [[nodiscard]] " + cls->getClassName() + final + base, true, [&] {
        m_scope = Scope::Private;
        if (cls->isRoot()) {
            scope(Scope::Public);
            line("NO_COPY_AND_MOVE(" + cls->getClassName() + ")", "");
            newline();
        }

        constructor(cls);
        functions(cls);
        classMembers(cls);
    });
    newline();
}

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
        doc("Create " + cls->getClassName() + " ast node");
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

void AstGen::functions(AstClass* cls) {
    const auto functions = cls->functions();
    if (functions.empty() && !cls->isRoot()) {
        return;
    }
    scope(Scope::Public);

    if (cls->isRoot()) {
        doc("Get ast node kind value");
        block("[[nodiscard]] constexpr auto getKind() const -> AstKind", [&] {
            line("return m_kind");
        });
        newline();
    }

    for (const auto& func : functions) {
        lines(func, "\n");
        newline();
    }
}

void AstGen::classMembers(AstClass* cls) {
    const auto members = cls->dataMembers();
    if (members.empty() && !cls->isRoot()) {
        return;
    }

    scope(Scope::Private);
    if (cls->isRoot()) {
        line("AstKind m_kind");
    }
    list(members, {});
}
