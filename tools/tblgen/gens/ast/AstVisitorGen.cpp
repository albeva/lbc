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
    doc("Constrains a visitor parameter to match the exact node type,\n"
        "stripping cv-ref qualifiers before comparison.");
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
    doc("Common base for all AST visitors.\n"
        "Provides unhandled() helpers for reporting missing accept() overloads during development.");
    block("class AstVisitorBase", true, [&] {
        scope(Scope::Public, true);
        line("virtual ~AstVisitorBase() = default");
        newline();
        scope(Scope::Protected);

        // by reference
        comment("Report an unhandled node and terminate. Call from a catch-all accept() to flag missing overloads.");
        block("[[noreturn]] static void unhandled(const AstRoot& ast, const std::source_location loc = std::source_location::current())", [&] {
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
        comment("Pointer overload - asserts non-null, then delegates to the reference version.");
        block("[[noreturn]] static void unhandled(const AstRoot* ast, const std::source_location loc = std::source_location::current())", [&] {
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
    getRoot()->visit([&](const AstClass* klass) {
        if (not klass->isLeaf()) {
            visitorClass(klass);
        }
    });
}

/**
 * Generate visitor class for given group
 */
void AstVisitorGen::visitorClass(const AstClass* ast) {
    std::string docStr;
    if (ast->isRoot()) {
        docStr = "Visitor that dispatches over all concrete AST nodes.";
    } else {
        docStr = "Visitor for " + ast->getRecord()->getValueAsString("desc").str()
            + " nodes under " + ast->getClassName() + ".";
    }
    docStr += "\n\n"
        "Inherit privately, friend the visitor, and implement accept() handlers.\n"
        "A generic accept(const auto&) catch-all can handle unimplemented nodes.\n\n";
    docStr += visitorSample(ast);
    doc(docStr);
    line("template <typename ReturnType = void>", "");
    block("class " + ast->getVisitorName() + " : AstVisitorBase", true, [&] {
        visit(ast);
    });
    newline();
}

void AstVisitorGen::visit(const AstClass* klass) {
    if (klass->getChildren().empty()) {
        return;
    }
    scope(Scope::Public, true);
    doc("Dispatch to the appropriate accept() handler based on the node's AstKind.");
    block("constexpr auto visit(this auto& self, IsNode<" + klass->getClassName() + "> auto& ast) -> ReturnType", [&] {
        block("switch (ast.getKind())", [&] {
            klass->visit(AstClass::Kind::Leaf, [&](const AstClass* node) {
                caseAccept(node);
            });
            defaultCase();
        });
    });
}

/**
 * Generate sample visitor code for use in the class documentation
 */
auto AstVisitorGen::visitorSample(const AstClass* klass) -> std::string {
    const auto sampleName = "Sample" + klass->getVisitorName().substr(3);
    const auto& visitorName = klass->getVisitorName();
    const auto& className = klass->getClassName();

    std::string sample;
    sample += "@code\n";
    sample += "class " + sampleName + " final : " + visitorName + "<> {\n";
    sample += "public:\n";
    sample += "    auto process(const " + className + "& ast) const {\n";
    sample += "        visit(ast);\n";
    sample += "    }\n";
    sample += "\n";
    sample += "private:\n";
    sample += "    friend " + visitorName + ";\n";
    sample += "\n";
    sample += "    void accept(const auto& ast) const {\n";
    sample += "        unhandled(ast);\n";
    sample += "    }\n";

    klass->visit(AstClass::Kind::Leaf, [&](const AstClass* node) {
        sample += "\n    // void accept(const " + node->getClassName() + "& ast) const;";
    });

    sample += "\n};\n";
    sample += "@endcode";

    return sample;
}

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
