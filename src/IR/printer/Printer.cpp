//
// Created by Albert Varaksin on 10/04/2026.
//
#include "Printer.hpp"
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

Printer::Printer(llvm::raw_ostream& output, const bool colors, const Style& style)
: m_output(output)
, m_style(style)
, m_colors(colors && output.has_colors()) {}

// =============================================================================
// Module / Function / Block
// =============================================================================

void Printer::print(const Module& module) const {
    // Global declarations
    for (auto* decl : module.getDeclarations()) {
        emitKeyword("declare");
        m_output << ' ';
        printInstruction(*decl);
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

void Printer::printFunction(const Function& func) const {
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

void Printer::printBlock(const BasicBlock& block) const {
    emitLabel(block);
    m_output << ":\n";

    for (const auto& instr : block.getBody()) {
        indent();
        printInstruction(instr);
    }
}

// =============================================================================
// Syntax element emitters
// =============================================================================

void Printer::emitKeyword(const llvm::StringRef text) const {
    color(m_style.keyword);
    m_output << text;
    reset();
}

void Printer::emitMnemonic(const Instruction& instr) const {
    color(m_style.mnemonic);
    m_output << instr.getMnemonic();
    reset();
}

void Printer::emitLocal(const NamedValue& value) const {
    color(m_style.local);
    m_output << '%' << value.getName();
    reset();
}

void Printer::emitGlobal(const NamedValue& value) const {
    color(m_style.global);
    m_output << '$' << value.getName();
    reset();
}

void Printer::emitLabel(const BasicBlock& block) const {
    color(m_style.label);
    m_output << '@' << block.getName();
    reset();
}

void Printer::emitType(const Type* type) const {
    color(m_style.type);
    m_output << type->string();
    reset();
}

void Printer::emitLiteral(const Value& value) const {
    const auto& lit = llvm::cast<Literal>(value);
    if (lit.getValue().isString()) {
        emitString(lit.getValue().get<llvm::StringRef>());
    } else {
        color(m_style.literal);
        m_output << lit.getValue().asString();
        reset();
    }
}

void Printer::emitString(const llvm::StringRef text) const {
    color(m_style.string);
    m_output << '"' << text << '"';
    reset();
}

void Printer::emitComment(const llvm::StringRef text) const {
    color(m_style.comment);
    m_output << text;
    reset();
}

void Printer::emitValue(const Value& value) const {
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

void Printer::indent() const {
    m_output << "  ";
}

// =============================================================================
// Color helpers
// =============================================================================

void Printer::color(const llvm::raw_ostream::Colors col) const {
    if (m_colors) {
        m_output.changeColor(col);
    }
}

void Printer::reset() const {
    if (m_colors) {
        m_output.resetColor();
    }
}
