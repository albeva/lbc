//
// Created by Albert Varaksin on 15/02/2026.
//
#include "AstVisitorGen.hpp"
using namespace std::string_literals;
using namespace ast;

AstVisitorGen::AstVisitorGen(
    raw_ostream& os,
    const RecordKeeper& records
)
: AstGen(os, records, genName, "lbc", { "pch.hpp", "Ast/Ast.hpp" }) {}

auto AstVisitorGen::run() -> bool {
    visitorBaseClass();
    visitorClasses();
    visitFunction();
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
        block("[[noreturn]] static void unhandled(const AstRoot& ast, const std::source_location& loc = std::source_location::current())", [&] {
            line("std::println(stderr, \"Unhandled {} at {}\", ast.getClassName(), loc)");
            line("std::exit(EXIT_FAILURE)");
        });
        newline();

        // by pointer
        comment("Pointer overload - asserts non-null, then delegates to the reference version.");
        block("[[noreturn]] static void unhandled(const AstRoot* ast, const std::source_location& loc = std::source_location::current())", [&] {
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
    const auto& visitorName = ast->getVisitorName();
    const auto& className = ast->getClassName();
    const auto sampleName = "Sample" + visitorName.substr(3);

    doc([&] {
        if (ast->isRoot()) {
            line("Visitor that dispatches over all concrete AST nodes.", "");
        } else {
            line("Visitor for "s + ast->getRecord()->getValueAsString("desc").str() + " nodes under " + className + ".", "");
        }
        newline();
        line("Inherit privately, friend the visitor, and implement accept() handlers.", "");
        line("A generic accept(const auto&) catch-all can handle unimplemented nodes.", "");
        newline();
        line("@code", "");
        block("class " + sampleName + " final : " + visitorName + "<>", true, [&] {
            scope(Scope::Public, true);
            block("auto process(const " + className + "& ast) const", [&] {
                line("visit(ast)");
            });
            newline();
            scope(Scope::Private);
            line("friend " + visitorName);
            newline();
            block("void accept(const auto& ast) const", [&] {
                line("unhandled(ast)");
            });
            newline();
            ast->visit(AstClass::Kind::Leaf, [&](const AstClass* node) {
                line("// void accept(const " + node->getClassName() + "& ast) const");
            });
        });
        line("@endcode", "");
    });

    line("template <typename ReturnType = void>", "");
    block("class " + visitorName + " : AstVisitorBase", true, [&] {
        scope(Scope::Public, true);
        comment("Result type of ast accept calls");
        line("using Result = ReturnType");
        newline();
        visit(ast);
    });
    newline();
}

void AstVisitorGen::visit(const AstClass* klass) {
    if (klass->getChildren().empty()) {
        return;
    }
    doc("Dispatch to the appropriate accept() handler based on the node's AstKind.");
    block("constexpr auto visit(this auto& self, std::derived_from<" + klass->getClassName() + "> auto& ast) -> Result", [&] {
        block("switch (ast.getKind())", [&] {
            klass->visit(AstClass::Kind::Leaf, [&](const AstClass* node) {
                caseAccept(node);
            });
            defaultCase();
        });
    });
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

/**
 * Generate free visit function for given group
 */
void AstVisitorGen::visitFunction() {
    const auto* root = getRoot();
    const auto childdoc = [&](const AstClass* child) {
        if (child->isLeaf()) {
            line("[&](const " + child->getClassName() + "& ast) {}", ",");
        }
    };
    doc([&] {
        line("Dispatch over concrete AST nodes using a callable visitor.", "");
        newline();
        line("@code", "");
        block("const auto visitor = Visitor", true, [&] {
            comment(root->getEnumName());
            for (const auto& child : root->getChildren()) {
                childdoc(child.get());
            }
            root->visit(AstClass::Kind::Group, [&](const AstClass* group) {
                comment(group->getEnumName());
                for (const auto& child : group->getChildren()) {
                    childdoc(child.get());
                }
            });
        });
        line("visit(ast, visitor)");
        line("@endcode", "");
    });

    line("template <typename Callable>", "");
    block("constexpr auto visit(std::derived_from<" + root->getClassName() + "> auto& ast, Callable&& callable) -> decltype(auto)", [&] {
        block("switch (ast.getKind())", [&] {
            root->visit(AstClass::Kind::Leaf, [&](const AstClass* node) {
                caseForward(node);
            });
            defaultCase();
        });
    });
}

/**
 * Generate case statement that forwards to the callable visitor
 */
void AstVisitorGen::caseForward(const AstClass* klass) {
    line("case AstKind::" + klass->getEnumName(), ":");
    line("    return std::forward<Callable>(callable)(llvm::cast<" + klass->getClassName() + ">(ast))");
}
