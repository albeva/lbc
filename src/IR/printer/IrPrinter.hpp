//
// Created by Albert Varaksin on 10/04/2026.
//
#pragma once
#include "pch.hpp"
namespace lbc::ir::lib {
class BasicBlock;
class CallInstr;
class CondJmpInstr;
class Function;
class Instruction;
class IrExpression;
class IrUnary;
class IrBinary;
class JmpInstr;
class Module;
class RetInstr;
class StoreInstr;
class RetainInstr;
class ReleaseInstr;
class Value;
class VarInstr;
class CastInstr;
class LoadInstr;
class AddrOfInstr;
} // namespace lbc::ir::lib
namespace lbc::ir::printer {

/**
 * Pretty-prints IR modules to an output stream with optional color syntax.
 *
 * Prefixes:
 *   % — local variables and temporaries
 *   @ — block labels (jump targets)
 *   $ — global symbols (functions)
 */
class IrPrinter final {
public:
    NO_COPY_AND_MOVE(IrPrinter)

    explicit IrPrinter(llvm::raw_ostream& output = llvm::outs(), bool colors = true);

    void print(const lib::Module& module) const;

private:
    void printFunction(const lib::Function& func) const;
    void printBlock(const lib::BasicBlock& block) const;
    void printInstruction(const lib::Instruction& instr) const;

    // Instructions
    void printVar(const lib::VarInstr& instr) const;
    void printStore(const lib::StoreInstr& instr) const;
    void printRetain(const lib::RetainInstr& instr) const;
    void printRelease(const lib::ReleaseInstr& instr) const;
    void printJmp(const lib::JmpInstr& instr) const;
    void printCondJmp(const lib::CondJmpInstr& instr) const;
    void printRet(const lib::RetInstr& instr) const;
    void printCast(const lib::CastInstr& instr) const;
    void printLoad(const lib::LoadInstr& instr) const;
    void printAddrOf(const lib::AddrOfInstr& instr) const;
    void printCall(const lib::CallInstr& instr) const;
    void printUnary(const lib::IrUnary& instr) const;
    void printBinary(const lib::IrBinary& instr) const;

    // Helpers
    void printValue(const lib::Value& value) const;
    void printMnemonic(const lib::Instruction& instr) const;
    void indent() const;
    void color(llvm::raw_ostream::Colors c) const;
    void reset() const;

    llvm::raw_ostream& m_output;
    bool m_colors;
};

} // namespace lbc::ir::printer
