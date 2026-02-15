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
    m_className = ("Ast" + record->getName()).str();
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

    // class args
    for (const auto& member : record->getValueAsListOfDefs("members")) {
        if (member->hasDirectSuperClass(gen.getArgClass())) {
            m_args.emplace_back(std::make_unique<AstArg>(member));
        } else if (member->hasDirectSuperClass(gen.getFuncClass())) {
            m_functions.emplace_back(unindent(member->getValueAsString("func")));
        }
    }
}

/**
 * In .td file, code may be too indented. Remove outermost
 * indentation along with leading and trailing blank lines.
 */
auto AstClass::unindent(llvm::StringRef code) -> std::string {
    SmallVector<StringRef, 16> lines;
    code.split(lines, '\n');

    // Strip leading and trailing blank lines
    while (!lines.empty() && lines.front().ltrim().empty()) {
        lines.erase(lines.begin());
    }
    while (!lines.empty() && lines.back().ltrim().empty()) {
        lines.pop_back();
    }

    if (lines.empty()) {
        return {};
    }

    // Find minimum indentation across non-empty lines
    auto minIndent = std::numeric_limits<std::size_t>::max();
    for (const auto& line : lines) {
        if (line.ltrim().empty()) {
            continue;
        }
        minIndent = std::min(minIndent, line.size() - line.ltrim().size());
    }

    // Remove common indentation and rejoin
    std::string result;
    for (std::size_t i = 0; i < lines.size(); i++) {
        if (i > 0) {
            result += '\n';
        }
        if (lines[i].ltrim().empty()) {
            continue;
        }
        result += lines[i].drop_front(minIndent);
    }
    return result;
}

auto AstClass::ctorParams() const -> std::vector<std::string> {
    std::vector<std::string> params;
    if (const auto* parent = m_parent) {
        params.append_range(parent->ctorParams());
    }
    for (const auto& arg : m_args) {
        if (arg->hasCtorParam()) {
            std::string param;
            if (arg->passAsConst()) {
                param += "const ";
            }
            param += arg->getType() + " " + arg->getName();
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
            for (const auto& arg : klass->m_args) {
                if (arg->hasCtorParam()) {
                    super.append(", ");
                    super.append(arg->getName());
                }
            }
        };
        collect(m_parent);
        init.emplace_back(m_parent->getClassName() + "(" + super + ")");
    }

    // class args
    for (const auto& args : m_args) {
        if (args->hasCtorParam()) {
            init.emplace_back("m_" + args->getName() + "(" + args->getName() + ")");
        }
    }

    return init;
}

auto AstClass::classArgs() const -> std::vector<std::string> {
    std::vector<std::string> args;
    for (const auto& arg : m_args) {
        std::string decl = arg->getType() + " m_" + arg->getName();
        if (not arg->getDefault().empty()) {
            decl += " = " + arg->getDefault();
        }
        decl += ";";
        args.emplace_back(decl);
    }
    return args;
}

auto AstClass::classFunctions() const -> std::vector<std::string> {
    std::vector<std::string> funcs;
    funcs.reserve((m_args.size() * 2) + m_functions.size());

    constexpr auto capitalizeFirst = [](const std::string& str) -> std::string {
        if (str.empty()) {
            return "";
        }

        std::string copy = str;
        copy.front() = llvm::toUpper(copy.front());
        return copy;
    };

    for (const auto& arg : m_args) {
        // getter
        funcs.emplace_back(
            "/// Get the "
            + (arg->getName() + "\n")
            + "[[nodiscard]] constexpr auto get"
            + capitalizeFirst(arg->getName())
            + "() const -> " + arg->getType() + " {\n"
            + "    return m_" + arg->getName() + ";\n"
            + "}"
        );

        // setter
        if (arg->hasSetter()) {
            funcs.emplace_back(
                "/// Set the "
                + (arg->getName() + "\n")
                + "void set"
                + capitalizeFirst(arg->getName()) + "("
                + (arg->passAsConst() ? "const " : "") + arg->getType() + " " + arg->getName() + ") {\n"
                + "    m_" + arg->getName() + " = " + arg->getName() + ";\n"
                + "}"
            );
        }
    }

    for (const auto& func : m_functions) {
        funcs.emplace_back(func);
    }

    return funcs;
}

auto AstClass::hasOwnCtorParams() -> bool {
    for (const auto& arg : m_args) {
        if (arg->hasCtorParam()) {
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
