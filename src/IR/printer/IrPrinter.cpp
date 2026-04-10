//
// Created by Albert Varaksin on 10/04/2026.
//
#include "IrPrinter.hpp"
#include "IR/lib/BasicBlock.hpp"
#include "IR/lib/Function.hpp"
#include "IR/lib/Literal.hpp"
#include "IR/lib/Module.hpp"
#include "IR/lib/Temporary.hpp"
#include "IR/lib/Variable.hpp"
#include "Type/Type.hpp"
#include "Utilities/Joiner.hpp"
using namespace lbc::ir::printer;
using namespace lbc::ir::lib;

IrPrinter::IrPrinter(llvm::raw_ostream& output, const bool colors)
: m_output(output)
, m_colors(colors && output.has_colors()) {}

void IrPrinter::print(const Module& module) const {
    // Global declarations
    for (auto* decl : module.getDeclarations()) {
        color(llvm::raw_ostream::CYAN);
        m_output << "declare ";
        reset();
        printVar(*llvm::cast<VarInstr>(decl));
        m_output << '\n';
    }

    // Global init block
    if (auto* block = module.getGlobalInitBlock()) {
        if (!block->getBody().empty()) {
            color(llvm::raw_ostream::BRIGHT_BLACK);
            m_output << "; global init\n";
            reset();
            for (const auto& instr : block->getBody()) {
                printInstruction(instr);
            }
            m_output << '\n';
        }
    }

    // Functions
    bool first = true;
    for (auto& func : module.getFunctions()) {
        if (!first) {
            m_output << '\n';
        }
        first = false;
        printFunction(func);
    }
}

void IrPrinter::printFunction(const Function& func) const {
    // Function signature
    color(llvm::raw_ostream::CYAN);
    m_output << "define ";
    reset();

    // Return type
    if (const auto* type = func.getType()) {
        color(llvm::raw_ostream::GREEN);
        m_output << type->string();
        reset();
        m_output << ' ';
    }

    // Function name
    color(llvm::raw_ostream::YELLOW);
    m_output << '$' << func.getName();
    reset();

    m_output << " {\n";

    // Blocks
    bool firstBlock = true;
    for (auto& block : func.getBlocks()) {
        if (!firstBlock) {
            m_output << '\n';
        }
        firstBlock = false;
        printBlock(block);
    }

    m_output << "}\n";
}

void IrPrinter::printBlock(const BasicBlock& block) const {
    // Block label
    color(llvm::raw_ostream::MAGENTA);
    m_output << '@' << block.getName();
    reset();
    m_output << ":\n";

    // Instructions
    for (const auto& instr : block.getBody()) {
        indent();
        printInstruction(instr);
    }
}

void IrPrinter::printInstruction(const Instruction& instr) const {
    switch (instr.getKind()) {
    case IrKind::Var:
        printVar(llvm::cast<VarInstr>(instr));
        break;
    case IrKind::Store:
        printStore(llvm::cast<StoreInstr>(instr));
        break;
    case IrKind::Retain:
        printRetain(llvm::cast<RetainInstr>(instr));
        break;
    case IrKind::Release:
        printRelease(llvm::cast<ReleaseInstr>(instr));
        break;
    case IrKind::Jmp:
        printJmp(llvm::cast<JmpInstr>(instr));
        break;
    case IrKind::CondJmp:
        printCondJmp(llvm::cast<CondJmpInstr>(instr));
        break;
    case IrKind::Ret:
        printRet(llvm::cast<RetInstr>(instr));
        break;
    case IrKind::Cast:
        printCast(llvm::cast<CastInstr>(instr));
        break;
    case IrKind::Load:
        printLoad(llvm::cast<LoadInstr>(instr));
        break;
    case IrKind::AddrOf:
        printAddrOf(llvm::cast<AddrOfInstr>(instr));
        break;
    case IrKind::Call:
        printCall(llvm::cast<CallInstr>(instr));
        break;
    case IrKind::Neg:
    case IrKind::Not:
        printUnary(llvm::cast<IrUnary>(instr));
        break;
    default:
        // All remaining are binary (arithmetic, logical, comparison)
        printBinary(llvm::cast<IrBinary>(instr));
        break;
    }
    m_output << '\n';
}

// result = var type
void IrPrinter::printVar(const VarInstr& instr) const {
    printValue(*instr.getResult());
    m_output << " = ";
    printMnemonic(instr);
    m_output << ' ';
    color(llvm::raw_ostream::GREEN);
    m_output << instr.getType()->string();
    reset();
}

// store dest, src
void IrPrinter::printStore(const StoreInstr& instr) const {
    printMnemonic(instr);
    m_output << ' ';
    printValue(*instr.getDest());
    m_output << ", ";
    printValue(*instr.getSrc());
}

// retain operand
void IrPrinter::printRetain(const RetainInstr& instr) const {
    printMnemonic(instr);
    m_output << ' ';
    printValue(*instr.getOperand());
}

// release operand
void IrPrinter::printRelease(const ReleaseInstr& instr) const {
    printMnemonic(instr);
    m_output << ' ';
    printValue(*instr.getOperand());
}

// jmp @label
void IrPrinter::printJmp(const JmpInstr& instr) const {
    printMnemonic(instr);
    m_output << ' ';
    color(llvm::raw_ostream::MAGENTA);
    m_output << '@' << llvm::cast<BasicBlock>(instr.getDestination())->getName();
    reset();
}

// jmp cond, @true, @false
void IrPrinter::printCondJmp(const CondJmpInstr& instr) const {
    printMnemonic(instr);
    m_output << ' ';
    printValue(*instr.getCondition());
    m_output << ", ";
    color(llvm::raw_ostream::MAGENTA);
    m_output << '@' << llvm::cast<BasicBlock>(instr.getTrueBlock())->getName();
    reset();
    m_output << ", ";
    color(llvm::raw_ostream::MAGENTA);
    m_output << '@' << llvm::cast<BasicBlock>(instr.getFalseBlock())->getName();
    reset();
}

// ret value
void IrPrinter::printRet(const RetInstr& instr) const {
    printMnemonic(instr);
    if (auto* value = instr.getValue()) {
        m_output << ' ';
        printValue(*value);
    }
}

// result = cast type operand
void IrPrinter::printCast(const CastInstr& instr) const {
    printValue(*instr.getResult());
    m_output << " = ";
    printMnemonic(instr);
    m_output << ' ';
    color(llvm::raw_ostream::GREEN);
    m_output << instr.getTargetType()->string();
    reset();
    m_output << ' ';
    printValue(*instr.getOperand());
}

// result = load source
void IrPrinter::printLoad(const LoadInstr& instr) const {
    printValue(*instr.getResult());
    m_output << " = ";
    printMnemonic(instr);
    m_output << ' ';
    printValue(*instr.getSource());
}

// result = addrof operand
void IrPrinter::printAddrOf(const AddrOfInstr& instr) const {
    printValue(*instr.getResult());
    m_output << " = ";
    printMnemonic(instr);
    m_output << ' ';
    printValue(*instr.getOperand());
}

// result = call $func(args...)
void IrPrinter::printCall(const CallInstr& instr) const {
    printValue(*instr.getResult());
    m_output << " = ";
    printMnemonic(instr);
    m_output << ' ';
    color(llvm::raw_ostream::YELLOW);
    m_output << '$' << instr.getCallee()->getName();
    reset();
    m_output << '(';
    Joiner joiner(m_output);
    for (const auto* arg : instr.getArgs()) {
        joiner();
        printValue(*arg);
    }
    m_output << ')';
}

// result = neg/not operand
void IrPrinter::printUnary(const IrUnary& instr) const {
    printValue(*instr.getResult());
    m_output << " = ";
    printMnemonic(instr);
    m_output << ' ';
    printValue(*instr.getOperand());
}

// result = op lhs, rhs
void IrPrinter::printBinary(const IrBinary& instr) const {
    printValue(*instr.getResult());
    m_output << " = ";
    printMnemonic(instr);
    m_output << ' ';
    printValue(*instr.getLhs());
    m_output << ", ";
    printValue(*instr.getRhs());
}

void IrPrinter::printValue(const Value& value) const {
    if (const auto* lit = llvm::dyn_cast<Literal>(&value)) {
        color(llvm::raw_ostream::RED);
        if (lit->getValue().isString()) {
            m_output << '"' << lit->getValue().get<llvm::StringRef>() << '"';
        } else {
            m_output << lit->getValue().asString();
        }
        reset();
        return;
    }

    const auto& named = llvm::cast<NamedValue>(value);
    if (llvm::isa<Function>(value)) {
        color(llvm::raw_ostream::YELLOW);
        m_output << '$' << named.getName();
    } else if (llvm::isa<BasicBlock>(value)) {
        color(llvm::raw_ostream::MAGENTA);
        m_output << '@' << named.getName();
    } else {
        color(llvm::raw_ostream::CYAN);
        m_output << '%' << named.getName();
    }
    reset();
}

void IrPrinter::printMnemonic(const Instruction& instr) const {
    color(llvm::raw_ostream::BLUE);
    m_output << instr.getMnemonic();
    reset();
}

void IrPrinter::indent() const {
    m_output << "  ";
}

void IrPrinter::color(const llvm::raw_ostream::Colors c) const {
    if (m_colors) {
        m_output.changeColor(c);
    }
}

void IrPrinter::reset() const {
    if (m_colors) {
        m_output.resetColor();
    }
}
