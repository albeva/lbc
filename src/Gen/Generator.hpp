//
// Created by Albert Varaksin on 15/06/2026.
//
#pragma once
#include "pch.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
namespace lbc {
class Type;
}
namespace lbc::ir::lib {
class Module;
class Function;
class BasicBlock;
class Instruction;
class Value;
class NamedValue;
class Literal;
} // namespace lbc::ir::lib

namespace lbc::gen {

/**
 * Lowers the analysed lbc IR (ir::lib) into an LLVM module.
 *
 * The lbc IR is intentionally not in SSA form: variables are named, mutable
 * storage and reads are implicit (no load/store encoded for them). This
 * generator materialises each variable as an `alloca`, loads on use and stores
 * on `store`, and leaves SSA construction (phi insertion) to LLVM's mem2reg.
 *
 * Implementation is split across Gen.cpp (module/function/block walk + value
 * helpers), GenType.cpp (type lowering), and GenInstr.cpp (instruction lowering).
 */
class Generator final {
public:
    NO_COPY_AND_MOVE(Generator)

    /** Construct over a caller-owned LLVM context that will own the result. */
    explicit Generator(llvm::LLVMContext& context);

    /** Lower the given IR module and hand back the resulting LLVM module. */
    [[nodiscard]] auto generate(const ir::lib::Module& module) -> std::unique_ptr<llvm::Module>;

private:
    // -------------------------------------------------------------------------
    // Walk (Gen.cpp)
    // -------------------------------------------------------------------------

    void lowerFunction(const ir::lib::Function& function);
    void lowerGlobalInit(const ir::lib::Module& module);
    void lowerBlock(const ir::lib::BasicBlock& block);

    // -------------------------------------------------------------------------
    // Instructions (GenInstr.cpp)
    // -------------------------------------------------------------------------

    void lowerInstruction(const ir::lib::Instruction& instr);

    // -------------------------------------------------------------------------
    // Values (Gen.cpp)
    // -------------------------------------------------------------------------

    /** Materialise a value for use as an operand — loads through variables. */
    [[nodiscard]] auto value(const ir::lib::Value* val) -> llvm::Value*;

    /** The storage address of a named value (its alloca / pointer), no load. */
    [[nodiscard]] auto address(const ir::lib::NamedValue* val) -> llvm::Value*;

    /** Lower a literal to an LLVM constant. */
    [[nodiscard]] auto literal(const ir::lib::Literal& lit) -> llvm::Constant*;

    /** Get or create the LLVM function for an IR function. */
    [[nodiscard]] auto function(const ir::lib::Function& fn) -> llvm::Function*;

    // -------------------------------------------------------------------------
    // Types (GenType.cpp)
    // -------------------------------------------------------------------------

    [[nodiscard]] auto lowerType(const Type* type) -> llvm::Type*;
    [[nodiscard]] auto lowerFunctionType(const Type* type) -> llvm::FunctionType*;
    [[nodiscard]] auto lowerCast(llvm::Value* val, const Type* from, const Type* to) -> llvm::Value*;

    /** Whether the (possibly const-qualified) type is a signed integral type. */
    [[nodiscard]] static auto isSigned(const Type* type) -> bool;

    // -------------------------------------------------------------------------
    // Data
    // -------------------------------------------------------------------------

    llvm::LLVMContext& m_llvm; ///< context that will own the produced module
    std::unique_ptr<llvm::Module> m_module;
    llvm::IRBuilder<> m_builder;
    llvm::Function* m_function = nullptr; ///< function currently being lowered
};

} // namespace lbc::gen
