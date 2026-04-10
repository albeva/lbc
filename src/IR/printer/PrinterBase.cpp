//
// Created by Albert Varaksin on 10/04/2026.
//
#include "IR/lib/BasicBlock.hpp"
#include "IR/lib/Function.hpp"
#include "IR/lib/Literal.hpp"
#include "IR/lib/Module.hpp"
#include "IR/lib/Temporary.hpp"
#include "IR/lib/Variable.hpp"
#include "PrinterBase.hpp"
#include "Type/Type.hpp"
#include "Utilities/Joiner.hpp"
using namespace lbc::ir::printer;
using namespace lbc::ir::lib;

PrinterBase::PrinterBase(llvm::raw_ostream& output, const bool colors, const Style style)
: m_output(output)
, m_style(style)
, m_colors(colors && output.has_colors()) {}

// =============================================================================
// Module / Function / Block
// =============================================================================

void PrinterBase::print(const Module& module) const {
    // Global declarations
    for (auto* decl : module.getDeclarations()) {
        emitKeyword("declare");
        m_output << ' ';
        printVar(*llvm::cast<VarInstr>(decl));
        m_output << '\n';
    }

    // Global init block
    if (auto* block = module.getGlobalInitBlock()) {
        if (!block->getBody().empty()) {
            emitComment("; global init");
            m_output << '\n';
            for (const auto& instr : block->getBody()) {
                printInstruction(instr);
            }
            m_output << '\n';
        }
    }

    // Functions
    bool first = true;
    for (const auto& func : module.getFunctions()) {
        if (!first) {
            m_output << '\n';
        }
        first = false;
        printFunction(func);
    }
}

void PrinterBase::printFunction(const Function& func) const {
    emitKeyword("define");
    m_output << ' ';

    if (const auto* type = func.getType()) {
        emitType(type);
        m_output << ' ';
    }

    emitGlobal(func);
    m_output << " {\n";

    bool firstBlock = true;
    for (const auto& block : func.getBlocks()) {
        if (!firstBlock) {
            m_output << '\n';
        }
        firstBlock = false;
        printBlock(block);
    }

    m_output << "}\n";
}

void PrinterBase::printBlock(const BasicBlock& block) const {
    emitLabel(block);
    m_output << ":\n";

    for (const auto& instr : block.getBody()) {
        indent();
        printInstruction(instr);
    }
}

// =============================================================================
// Instruction dispatch
// =============================================================================

void PrinterBase::printInstruction(const Instruction& instr) const {
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
        printBinary(llvm::cast<IrBinary>(instr));
        break;
    }
    m_output << '\n';
}

// =============================================================================
// Individual instructions
// =============================================================================

// %x = var INTEGER
void PrinterBase::printVar(const VarInstr& instr) const {
    emitValue(*instr.getResult());
    m_output << " = ";
    emitMnemonic(instr);
    m_output << ' ';
    emitType(instr.getType());
}

// store %x, 42
void PrinterBase::printStore(const StoreInstr& instr) const {
    emitMnemonic(instr);
    m_output << ' ';
    emitValue(*instr.getDest());
    m_output << ", ";
    emitValue(*instr.getSrc());
}

// retain %x
void PrinterBase::printRetain(const RetainInstr& instr) const {
    emitMnemonic(instr);
    m_output << ' ';
    emitValue(*instr.getOperand());
}

// release %x
void PrinterBase::printRelease(const ReleaseInstr& instr) const {
    emitMnemonic(instr);
    m_output << ' ';
    emitValue(*instr.getOperand());
}

// jmp @label
void PrinterBase::printJmp(const JmpInstr& instr) const {
    emitMnemonic(instr);
    m_output << ' ';
    emitLabel(llvm::cast<BasicBlock>(*instr.getDestination()));
}

// jmp %cond, @true, @false
void PrinterBase::printCondJmp(const CondJmpInstr& instr) const {
    emitMnemonic(instr);
    m_output << ' ';
    emitValue(*instr.getCondition());
    m_output << ", ";
    emitLabel(llvm::cast<BasicBlock>(*instr.getTrueBlock()));
    m_output << ", ";
    emitLabel(llvm::cast<BasicBlock>(*instr.getFalseBlock()));
}

// ret %x
void PrinterBase::printRet(const RetInstr& instr) const {
    emitMnemonic(instr);
    if (const auto* value = instr.getValue()) {
        m_output << ' ';
        emitValue(*value);
    }
}

// %0 = cast BYTE %x
void PrinterBase::printCast(const CastInstr& instr) const {
    emitValue(*instr.getResult());
    m_output << " = ";
    emitMnemonic(instr);
    m_output << ' ';
    emitType(instr.getTargetType());
    m_output << ' ';
    emitValue(*instr.getOperand());
}

// %0 = load %ptr
void PrinterBase::printLoad(const LoadInstr& instr) const {
    emitValue(*instr.getResult());
    m_output << " = ";
    emitMnemonic(instr);
    m_output << ' ';
    emitValue(*instr.getSource());
}

// %0 = addrof %x
void PrinterBase::printAddrOf(const AddrOfInstr& instr) const {
    emitValue(*instr.getResult());
    m_output << " = ";
    emitMnemonic(instr);
    m_output << ' ';
    emitValue(*instr.getOperand());
}

// %0 = call $foo(%x, %y)
void PrinterBase::printCall(const CallInstr& instr) const {
    emitValue(*instr.getResult());
    m_output << " = ";
    emitMnemonic(instr);
    m_output << ' ';
    emitGlobal(*instr.getCallee());
    m_output << '(';
    Joiner joiner(m_output);
    for (const auto* arg : instr.getArgs()) {
        joiner();
        emitValue(*arg);
    }
    m_output << ')';
}

// %0 = neg %x
void PrinterBase::printUnary(const IrUnary& instr) const {
    emitValue(*instr.getResult());
    m_output << " = ";
    emitMnemonic(instr);
    m_output << ' ';
    emitValue(*instr.getOperand());
}

// %0 = add %x, %y
void PrinterBase::printBinary(const IrBinary& instr) const {
    emitValue(*instr.getResult());
    m_output << " = ";
    emitMnemonic(instr);
    m_output << ' ';
    emitValue(*instr.getLhs());
    m_output << ", ";
    emitValue(*instr.getRhs());
}

// =============================================================================
// Syntax element emitters
// =============================================================================

void PrinterBase::emitKeyword(const llvm::StringRef text) const {
    color(m_style.keyword);
    m_output << text;
    reset();
}

void PrinterBase::emitMnemonic(const Instruction& instr) const {
    color(m_style.mnemonic);
    m_output << instr.getMnemonic();
    reset();
}

void PrinterBase::emitLocal(const NamedValue& value) const {
    color(m_style.local);
    m_output << '%' << value.getName();
    reset();
}

void PrinterBase::emitGlobal(const NamedValue& value) const {
    color(m_style.global);
    m_output << '$' << value.getName();
    reset();
}

void PrinterBase::emitLabel(const BasicBlock& block) const {
    color(m_style.label);
    m_output << '@' << block.getName();
    reset();
}

void PrinterBase::emitType(const Type* type) const {
    color(m_style.type);
    m_output << type->string();
    reset();
}

void PrinterBase::emitLiteral(const Value& value) const {
    const auto& lit = llvm::cast<Literal>(value);
    if (lit.getValue().isString()) {
        emitString(lit.getValue().get<llvm::StringRef>());
    } else {
        color(m_style.literal);
        m_output << lit.getValue().asString();
        reset();
    }
}

void PrinterBase::emitString(const llvm::StringRef text) const {
    color(m_style.string);
    m_output << '"' << text << '"';
    reset();
}

void PrinterBase::emitComment(const llvm::StringRef text) const {
    color(m_style.comment);
    m_output << text;
    reset();
}

void PrinterBase::emitValue(const Value& value) const {
    if (llvm::isa<Literal>(value)) {
        emitLiteral(value);
    } else if (llvm::isa<Function>(value)) {
        emitGlobal(llvm::cast<NamedValue>(value));
    } else if (llvm::isa<BasicBlock>(value)) {
        emitLabel(llvm::cast<BasicBlock>(value));
    } else {
        emitLocal(llvm::cast<NamedValue>(value));
    }
}

void PrinterBase::indent() const {
    m_output << "  ";
}

// =============================================================================
// Color helpers
// =============================================================================

void PrinterBase::color(const llvm::raw_ostream::Colors col) const {
    if (m_colors) {
        m_output.changeColor(col);
    }
}

void PrinterBase::reset() const {
    if (m_colors) {
        m_output.resetColor();
    }
}
