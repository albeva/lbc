//
// Created by Albert Varaksin on 10/04/2026.
//
#pragma once
#include "pch.hpp"
namespace lbc {
class Type;
}
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
class NamedValue;
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
class Printer {
public:
    NO_COPY_AND_MOVE(Printer)

    /**
     * Color table for IR syntax highlighting.
     * Each field maps a syntactic role to an output color.
     */
    struct Style final {
        using Color = llvm::raw_ostream::Colors;
        Color keyword;  ///< define, declare
        Color mnemonic; ///< var, store, add, ret, jmp
        Color local;    ///< %x, %0 (variables and temporaries)
        Color global;   ///< $main (functions)
        Color label;    ///< entry, if.0.then (block labels)
        Color type;     ///< INTEGER, DOUBLE
        Color literal;  ///< 42, true, null
        Color string;   ///< "hello"
        Color comment;  ///< ; comment

        /** Default color scheme. */
        [[nodiscard]] static constexpr auto defaults() -> Style {
            return {
                .keyword = Color::BRIGHT_MAGENTA,
                .mnemonic = Color::BLUE,
                .local = Color::CYAN,
                .global = Color::YELLOW,
                .label = Color::MAGENTA,
                .type = Color::GREEN,
                .literal = Color::BRIGHT_RED,
                .string = Color::RED,
                .comment = Color::BRIGHT_BLACK,
            };
        }
    };

    explicit Printer(
        llvm::raw_ostream& output = llvm::outs(),
        bool colors = true,
        const Style& style = Style::defaults()
    );

    void print(const lib::Module& module) const;

    /** Get the output stream. */
    [[nodiscard]] auto output() const -> llvm::raw_ostream& { return m_output; }

    // Syntax element emitters
    void emitKeyword(llvm::StringRef text) const;
    void emitMnemonic(const lib::Instruction& instr) const;
    void emitLocal(const lib::NamedValue& value) const;
    void emitGlobal(const lib::NamedValue& value) const;
    void emitLabel(const lib::BasicBlock& block) const;
    void emitType(const Type* type) const;
    void emitLiteral(const lib::Value& value) const;
    void emitString(llvm::StringRef text) const;
    void emitComment(llvm::StringRef text) const;
    void emitValue(const lib::Value& value) const;
    void indent() const;

private:
    void printFunction(const lib::Function& func) const;
    void printBlock(const lib::BasicBlock& block) const;
    void printInstruction(const lib::Instruction& instr) const;

    void color(llvm::raw_ostream::Colors col) const;
    void reset() const;

    llvm::raw_ostream& m_output;
    Style m_style;
    bool m_colors;
};

} // namespace lbc::ir::printer
