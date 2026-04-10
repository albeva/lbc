//
// Created by Albert Varaksin on 11/04/2026.
//
#include "IrPrinterGen.hpp"
using namespace ir;

IrPrinterGen::IrPrinterGen(
    raw_ostream& os,
    const RecordKeeper& records
)
: IrGen(os, records, genName, "lbc::ir::printer", {}) {}

auto IrPrinterGen::run() -> bool {
    m_os << "//\n";
    m_os << "// DO NOT MODIFY. This file is AUTO GENERATED.\n";
    m_os << "//\n";
    m_os << "// clang-format off\n";
    m_os << "#include \"Printer.hpp\"\n";
    m_os << "#include \"IR/lib/BasicBlock.hpp\"\n";
    m_os << "#include \"IR/lib/Function.hpp\"\n";
    m_os << "#include \"IR/lib/Literal.hpp\"\n";
    m_os << "#include \"IR/lib/Temporary.hpp\"\n";
    m_os << "#include \"IR/lib/Variable.hpp\"\n";
    m_os << "#include \"Type/Type.hpp\"\n";
    m_os << "#include \"Utilities/Joiner.hpp\"\n";
    m_os << "using namespace lbc::ir::printer;\n";
    m_os << "using namespace lbc::ir::lib;\n";
    m_os << "\n";

    // Forward declarations of per-instruction helpers
    m_os << "namespace {\n";
    getRoot()->visit(lib::TreeNode::Kind::Leaf, [&](const lib::TreeNode* node) {
        const auto* ir = static_cast<const IrNodeClass*>(node); // NOLINT(*-static-cast-downcast)
        m_os << "void print" << ir->getEnumName() << "(const Printer&, const " << ir->getClassName() << "&);\n";
    });
    m_os << "} // namespace\n\n";

    // Dispatch method
    dispatch();

    // Per-instruction free functions
    m_os << "\nnamespace {\n";
    getRoot()->visit(lib::TreeNode::Kind::Leaf, [&](const lib::TreeNode* node) {
        m_os << "\n";
        printInstruction(static_cast<const IrNodeClass*>(node)); // NOLINT(*-static-cast-downcast)
    });
    m_os << "\n} // namespace\n";

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
    for (const auto* current = node; current != nullptr; current = current->getParent()) {
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
