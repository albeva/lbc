//
// Created by Albert Varaksin on 15/02/2026.
//
#include "AstVisitorGen.hpp"

AstVisitorGen::AstVisitorGen(
    raw_ostream& os,
    const RecordKeeper& records
)
: AstGen(os, records, genName, "lbc", { "Ast/Ast.hpp" }) { }

auto AstVisitorGen::run() -> bool {
    m_os << "template<typename T, typename U>\n"
            "concept IsNode = std::same_as<std::remove_cvref_t<T>, U>;\n\n";

    visitorBaseClass();
    visitorClasses();
    return false;
}

/**
 * Generate AstVisitorBase class
 */
void AstVisitorGen::visitorBaseClass() {
    doc("This is a base class for all Ast Visitors");
    block("class AstVisitorBase", true, [&] {
        scope(Scope::Public, true);
        line("virtual ~AstVisitorBase() = default");
        newline();
        scope(Scope::Protected);

        // by reference
        comment("Print out information about unhandled node and terminate");
        block("[[noreturn]] constexpr static void unhandled(const AstRoot& ast, const std::source_location loc = std::source_location::current())", [&] {
            lines(
                "std::println(\n"
                "    stderr,\n"
                "    \"Unhandled {} at {}:{}:{} in {}\",\n"
                "    std::string_view(ast.getClassName()),\n"
                "    loc.file_name(),\n"
                "    loc.line(),\n"
                "    loc.column(),\n"
                "    loc.function_name()\n"
                ");\n"
                "std::exit(EXIT_FAILURE);"
            );
        });
        newline();

        // by pointer
        comment("Print out information about unhandled node and terminate");
        block("[[noreturn]] constexpr static void unhandled(const AstRoot* ast, const std::source_location loc = std::source_location::current())", [&] {
            line("assert(ast != nullptr)");
            line("unhandled(*ast, loc)");
        });
    });
    newline();
}

/**
 * Emit visitors for all ast groups
 */
void AstVisitorGen::visitorClasses() {
    const auto recurse = [&](this auto&& self, const AstClass* klass) -> void {
        if (klass->isLeaf()) {
            return;
        }
        visitorClass(klass);
        for (const auto& child : klass->getChildren()) {
            self(child.get());
        }
    };
    recurse(getRoot());
}

/**
 * Generate visitor class for given group
 */
void AstVisitorGen::visitorClass(const AstClass* ast) {
    line("template <typename ReturnType = void>", "");
    block("class " + ast->getVisitorName() + " : AstVisitorBase", true, [&] {
        visit(ast);

        const auto recurse = [&](this auto&& self, const AstClass* node) -> void {
            for (const auto& child : node->getChildren()) {
                if (not child->isGroup()) {
                    continue;
                }
                newline();
                // forward(child.get(), ast); // NOLINT(*-suspicious-call-argument)
                self(child.get());
            }
        };
        recurse(ast);
    });
    newline();
}

void AstVisitorGen::visit(const AstClass* klass) {
    if (klass->getChildren().empty()) {
        return;
    }
    scope(Scope::Public, true);
    block("constexpr auto visit(this auto& self, IsNode<" + klass->getClassName() + "> auto& ast) -> ReturnType", [&] {
        block("switch (ast.getKind())", [&] {
            klass->visit(AstClass::Kind::Leaf, [&](const AstClass* node) {
                caseAccept(node);
            });
            defaultCase();
        });
    });
}

// void AstVisitorGen::forward(const AstClass* klass, const AstClass* base) {
//     block("constexpr auto visit(this auto& self, IsNode<" + klass->getClassName() + "> auto& ast) -> ReturnType", [&] {
//         line("if constexpr (std::is_const_v<decltype(ast)>) {", "");
//         line("    return self.visit(static_cast<const " + base->getClassName() + "&>(ast))");
//         line("} else {", "");
//         line("    return self.visit(static_cast<" + base->getClassName() + "&>(ast))");
//         line("}", "");
//     });
// }

/**
 * Generate case statement for given node
 */
void AstVisitorGen::caseAccept(const AstClass* klass) {
    line("case AstKind::" + klass->getEnumName(), ":");
    line("    return self.accept(llvm::cast<" + klass->getClassName() + ">(ast))");
}

/**
 * Generate default case
 */
void AstVisitorGen::defaultCase() {
    line("default", ":");
    line("    std::unreachable()");
}
