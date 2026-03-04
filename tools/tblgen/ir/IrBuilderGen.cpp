//
// Created by Albert Varaksin on 01/03/2026.
//
#include "IrBuilderGen.hpp"
using namespace ir;

IrBuilderGen::IrBuilderGen(
    raw_ostream& os,
    const RecordKeeper& records
)
: IrGen(os, records, genName, "lbc::ir", { "pch.hpp", "Driver/Context.hpp", "Instructions.hpp" }) {}

auto IrBuilderGen::run() -> bool {
    builderClass();
    return false;
}

void IrBuilderGen::builderClass() {
    doc("Build IR instructions");
    klass("Builder", {}, [&] {
        scope(Scope::Public, true);
        line("NO_COPY_AND_MOVE(Builder)");
        newline();

        line("explicit Builder(Context& context)", "");
        line(": m_context(context) {}", "");
        newline();

        getter("context", "Context&", false);

        builders();
        newline();

        scope(Scope::Private);
        line("Context& m_context");
    });
}

void IrBuilderGen::builders() {
    getRoot()->visit(lib::TreeNode::Kind::Leaf, [&](const lib::TreeNode* node) {
        newline();
        builder(static_cast<const IrNodeClass*>(node)); // NOLINT(*-static-cast-downcast)
    });
}

void IrBuilderGen::builder(const IrNodeClass* node) {
    const auto name = "make" + ucfirst(node->getMnemonic());
    const auto params = join(node->ctorParams());

    doc(node->getRecord()->getValueAsString("desc"));
    block("[[nodiscard]] auto " + name + "(" + params + ") const -> " + node->getClassName() + "*", [&] {
        const auto args = join(node->ctorArgs());
        line("return m_context.create<" + node->getClassName() + ">(" + args + ")");
    });
}
