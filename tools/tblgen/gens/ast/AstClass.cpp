//
// Created by Albert Varaksin on 15/02/2026.
//
#include "AstClass.hpp"
#include "AstGen.hpp"
#include <llvm/ADT/StringExtras.h>
#include <llvm/TableGen/Record.h>
using namespace llvm;

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
