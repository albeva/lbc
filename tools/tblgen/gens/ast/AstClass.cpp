//
// Created by Albert Varaksin on 15/02/2026.
//
#include "AstClass.hpp"
#include <llvm/ADT/StringExtras.h>
#include <llvm/TableGen/Record.h>
#include "AstGen.hpp"
using namespace llvm;

AstClass::AstClass(AstClass* parent, const AstGen& gen, const Record* record)
: m_parent(parent)
, m_record(record) {
    m_className = std::string("Ast").append(record->getName());
    m_enumName = record->getName();

    // class children
    if (record->hasDirectSuperClass(gen.getGroupClass())) {
        m_kind = parent == nullptr ? Kind::Root : Kind::Group;

        // child records
        auto children = AstGen::collect(gen.getNodeRecords(), "parent", record);

        // add to children
        m_children.reserve(children.size());
        for (const auto& child : children) {
            m_children.emplace_back(std::make_unique<AstClass>(this, gen, child));
        }

        // pull leaves to top
        std::ranges::stable_partition(m_children, [&](const std::unique_ptr<AstClass>& item) {
            return item->isLeaf();
        });
    } else {
        m_kind = Kind::Leaf;
    }

    // class members
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
            "/// Get the "
            + (member->getName() + "\n")
            + "[[nodiscard]] constexpr auto get"
            + capitalizeFirst(member->getName())
            + "() const -> " + member->getType() + " {\n"
            + "    return m_" + member->getName() + ";\n"
            + "}"
        );

        // setter
        if (member->hasSetter()) {
            funcs.emplace_back(
                "/// Set the "
                + (member->getName() + "\n")
                + "void set"
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

auto AstClass::getVisitorName() const -> std::string {
    if (isRoot()) {
        return "AstVisitor";
    }
    return m_className + "Visitor";
}

auto AstClass::getLeafRange() const -> std::optional<std::pair<const AstClass*, const AstClass*>> {
    const AstClass* first = nullptr;
    const AstClass* last = nullptr;

    visit(Kind::Leaf, [&](const AstClass* node) -> void {
        if (first == nullptr) {
            first = node;
        }
        last = node;
    });

    if (first == nullptr) {
        return std::nullopt;
    }
    return std::pair { first, last };
}
