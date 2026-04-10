//
// Created by Albert Varaksin on 11/04/2026.
//
#include "IrPrinterGen.hpp"
using namespace ir;

IrPrinterGen::IrPrinterGen(
    raw_ostream& os,
    const RecordKeeper& records
)
: IrGen(os, records, genName, "lbc::ir::printer", { "\"Printer.hpp\"", "\"IR/lib/BasicBlock.hpp\"", "\"IR/lib/Function.hpp\"", "\"IR/lib/Literal.hpp\"", "\"IR/lib/Temporary.hpp\"", "\"IR/lib/Variable.hpp\"", "\"Type/Type.hpp\"", "\"Utilities/Joiner.hpp\"" }) {}

auto IrPrinterGen::run() -> bool {
    header({ .pragmaOnce = false, .openNamespace = false });
    line("using namespace lbc::ir::printer");
    line("using namespace lbc::ir::lib");
    newline();

    // Anonymous namespace with forward declarations and implementations
    block("namespace", [&] {
        // Forward declarations
        getRoot()->visit(lib::TreeNode::Kind::Leaf, [&](const lib::TreeNode* node) {
            const auto* ir = static_cast<const IrNodeClass*>(node); // NOLINT(*-static-cast-downcast)
            line("void print" + ir->getEnumName() + "(const Printer&, const " + ir->getClassName() + "&)");
        });
    });
    newline();

    // Dispatch method
    dispatch();
    newline();

    // Per-instruction implementations in anonymous namespace
    block("namespace", [&] {
        getRoot()->visit(lib::TreeNode::Kind::Leaf, [&](const lib::TreeNode* node) {
            newline();
            printInstruction(static_cast<const IrNodeClass*>(node)); // NOLINT(*-static-cast-downcast)
        });
    });
    return false;
}

void IrPrinterGen::dispatch() {
    block("void Printer::printInstruction(const Instruction& instr) const", [&] {
        block("switch (instr.getKind())", [&] {
            getRoot()->visit(lib::TreeNode::Kind::Leaf, [&](const lib::TreeNode* node) {
                const auto* ir = static_cast<const IrNodeClass*>(node); // NOLINT(*-static-cast-downcast)
                const auto enumName = ir->getEnumName();
                const auto className = ir->getClassName();
                line("case IrKind::" + enumName + ":", "");
                indent(false, [&] {
                    line("print" + enumName + "(*this, llvm::cast<" + className + ">(instr))");
                    line("break");
                });
            });
        });
        line(R"(m_output << '\n')");
    });
}

void IrPrinterGen::printInstruction(const IrNodeClass* node) {
    const auto className = node->getClassName();
    const auto methodName = "print" + node->getEnumName();
    const auto result = hasResult(node);

    // Collect all args from the parent chain, skipping "result"
    std::vector<const lib::TreeNodeArg*> allArgs;
    for (const lib::TreeNode* current = node; current != nullptr; current = current->getParent()) {
        for (const auto& arg : current->getArgs()) {
            if (arg->getName() != "result") {
                allArgs.push_back(arg.get());
            }
        }
    }

    block("void " + methodName + "(const Printer& p, const " + className + "& instr)", [&] {
        if (result) {
            line("p.emitValue(*instr.getResult())");
            line(R"(p.output() << " = ")");
        }

        line("p.emitMnemonic(instr)");

        bool firstArg = true;
        for (const auto* arg : allArgs) {
            const auto& type = arg->getType();
            const auto getter = "instr.get" + ucfirst(arg->getName()) + "()";

            if (type.contains("span")) {
                line(R"(p.output() << '(')");
                line("lbc::Joiner joiner(p.output())");
                block("for (const auto* arg : " + getter + ")", [&] {
                    line("joiner()");
                    line("p.emitValue(*arg)");
                });
                line(R"(p.output() << ')')");
                continue;
            }

            if (firstArg) {
                line(R"(p.output() << ' ')");
                firstArg = false;
            } else {
                line(R"(p.output() << ", ")");
            }

            line("p." + emitCall(*arg));
        }
    });
}

auto IrPrinterGen::hasResult(const lib::TreeNode* node) -> bool {
    for (const lib::TreeNode* current = node; current != nullptr; current = current->getParent()) {
        for (const auto& arg : current->getArgs()) {
            if (arg->getName() == "result") {
                return true;
            }
        }
    }
    return false;
}

auto IrPrinterGen::emitCall(const lib::TreeNodeArg& arg) -> std::string {
    const auto& type = arg.getType();
    const auto getter = "instr.get" + ucfirst(arg.getName()) + "()";

    if (type == "BasicBlock*") {
        return "emitLabel(llvm::cast<BasicBlock>(*" + getter + "))";
    }
    if (type == "Function*") {
        return "emitGlobal(*" + getter + ")";
    }
    if (type.contains("Type*")) {
        return "emitType(" + getter + ")";
    }
    return "emitValue(*" + getter + ")";
}
