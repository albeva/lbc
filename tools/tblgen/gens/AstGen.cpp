// Custom TableGen backend for generating AST node definitions.
// Reads Ast.td and emits Ast.inc
#include "AstGen.hpp"

// -----------------------------------------------------------------------------
// Ast class member
// -----------------------------------------------------------------------------

AstMember::AstMember(const Record* record)
: m_name(record->getValueAsString("name"))
, m_type(record->getValueAsString("type"))
, m_default(record->getValueAsString("default"))
, m_mutable(record->getValueAsBit("mutable"))
, m_ctorParam(m_default.empty()) { }

// -----------------------------------------------------------------------------
// AstClass
// -----------------------------------------------------------------------------

AstClass::AstClass(AstClass* parent, const AstGen& gen, const Record* record)
: m_parent(parent)
, m_record(record) {
    m_className = std::string("Ast").append(record->getName());
    m_enumName = record->getName();

    if (record->hasDirectSuperClass(gen.getGroupClass())) {
        if (const auto children = gen.getMap().find(record); children != gen.getMap().end()) {
            m_children.reserve(children->second.size());
            for (const Record* child : children->second) {
                m_children.emplace_back(std::make_unique<AstClass>(this, gen, child));
            }
        }
    }

    for (const auto& member : record->getValueAsListOfDefs("members")) {
        m_members.emplace_back(std::make_unique<AstMember>(member));
    }
}

auto AstClass::ctorParams() const -> std::vector<std::string> {
    std::vector<std::string> params;
    if (const auto* parent = m_parent) {
        params.append_range(parent->ctorParams());
    }
    for (const auto& member : m_members) {
        if (member->hasCtorParam()) {
            std::string param;
            if (member->passAsConst()) {
                param += "const ";
            }
            param += member->getType() + " " + member->getName();
            params.emplace_back(std::move(param));
        }
    }
    return params;
}

auto AstClass::ctorInitParams() const -> std::vector<std::string> {
    std::vector<std::string> init;

    // super class args
    if (m_parent != nullptr) {
        std::string super;
        const auto collect = [&](this auto&& self, const AstClass* klass) -> void {
            if (const auto* parent = klass->getParent()) {
                self(parent);
            } else {
                if (isGroup()) {
                    super = "kind";
                } else {
                    super = "AstKind::" + getEnumName();
                }
            }
            for (const auto& members : klass->m_members) {
                if (members->hasCtorParam()) {
                    super.append(", ");
                    super.append(members->getName());
                }
            }
        };
        collect(m_parent);
        init.emplace_back(m_parent->getClassName() + "(" + super + ")");
    }

    // class members
    for (const auto& member : m_members) {
        if (member->hasCtorParam()) {
            init.emplace_back("m_" + member->getName() + "(" + member->getName() + ")");
        }
    }

    return init;
}

auto AstClass::dataMembers() const -> std::vector<std::string> {
    std::vector<std::string> members;
    for (const auto& member : m_members) {
        std::string decl = member->getType() + " m_" + member->getName();
        if (not member->getDefault().empty()) {
            decl += " = " + member->getDefault();
        }
        decl += ";";
        members.emplace_back(decl);
    }
    return members;
}

auto AstClass::functions() const -> std::vector<std::string> {
    std::vector<std::string> funcs;
    funcs.reserve(m_members.size());

    constexpr auto capitalizeFirst = [](const std::string& str) -> std::string {
        if (str.empty()) {
            return "";
        }

        std::string copy = str;
        copy.front() = llvm::toUpper(copy.front());
        return copy;
    };

    for (const auto& member : m_members) {
        // getter
        funcs.emplace_back(
            "/**\n"
            " * Get "
            + (member->getName() + "\n")
            + " */\n"
              "[[nodiscard]] constexpr auto get"
            + capitalizeFirst(member->getName())
            + "() const -> " + member->getType() + " {\n"
            + "    return m_" + member->getName() + ";\n"
            + "}"
        );

        // setter
        if (member->hasSetter()) {
            funcs.emplace_back(
                "/**\n"
                " * Set "
                + (member->getName() + "\n")
                + " */\n"
                  "void set"
                + capitalizeFirst(member->getName()) + "("
                + (member->passAsConst() ? "const " : "") + member->getType() + " " + member->getName() + ") {\n"
                + "    m_" + member->getName() + " = " + member->getName() + ";\n"
                + "}"
            );
        }
    }

    return funcs;
}

auto AstClass::hasOwnCtorParams() -> bool {
    for (const auto& member : m_members) {
        if (member->hasCtorParam()) {
            return true;
        }
    }
    return false;
}

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
        doc("Create " + cls->getClassName() + " ast node");
        line("constexpr " + cls->getClassName() + "(", "");
        indent(false, [&] {
            if (cls->isRoot() || cls->isGroup()) {
                line("const AstKind kind", ",");
            }
            list(cls->ctorParams(), { .suffix = "," });
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
