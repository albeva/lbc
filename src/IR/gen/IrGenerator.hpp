//
// Created by Albert Varaksin on 08/03/2026.
//
#pragma once
#include "pch.hpp"
#include "Ast/Ast.hpp"
#include "Ast/AstVisitor.hpp"
#include "Diag/DiagEngine.hpp"
#include "Diag/LogProvider.hpp"
#include "IR/lib/Builder.hpp"
namespace lbc::ir::lib {
class BasicBlock;
class Function;
class Module;
class Temporary;
} // namespace lbc::ir::lib
namespace lbc::ir::gen {

/**
 * IR generator. Lowers the analysed AST into IR instructions.
 *
 * Implementation is split across multiple .cpp files by concern:
 * GenDecl, GenExpr, GenStmt, GenType, and Gen.cpp for common utilities.
 */
class IrGenerator final : AstVisitor<DiagResult<void>>, lib::Builder, LogProvider {
public:
    NO_COPY_AND_MOVE(IrGenerator)

    /**
     * Construct an IR generator for the given context.
     */
    explicit IrGenerator(Context& context);
    ~IrGenerator() override;

    /**
     * Generate IR from the given AST module.
     */
    [[nodiscard]] auto generate(const AstModule& ast) -> DiagResult<lib::Module*>;

    /**
     * Get associated context object.
     */
    using Builder::getContext;

private:
    friend AstVisitor;

    /** Generate IR for the module root node. */
    [[nodiscard]] auto accept(const AstModule& ast) -> Result;

    // -------------------------------------------------------------------------
    // Declarations (GenDecl.cpp)
    // -------------------------------------------------------------------------

    /** Generate IR for a variable declaration. */
    [[nodiscard]] auto accept(const AstVarDecl& ast) -> Result;

    /** Generate IR for a function declaration. */
    [[nodiscard]] auto accept(const AstFuncDecl& ast) -> Result;

    /** Generate IR for a function parameter declaration. */
    [[nodiscard]] auto accept(const AstFuncParamDecl& ast) -> Result;

    // -------------------------------------------------------------------------
    // Statements (GenStmt.cpp)
    // -------------------------------------------------------------------------

    /** Generate IR for a statement list. */
    [[nodiscard]] auto accept(const AstStmtList& ast) -> Result;

    /** Generate IR for an expression statement. */
    [[nodiscard]] auto accept(const AstExprStmt& ast) -> Result;

    /** Generate IR for a DECLARE statement. */
    [[nodiscard]] static auto accept(const AstDeclareStmt& ast) -> Result;

    /** Generate IR for a function body statement. */
    [[nodiscard]] auto accept(const AstFuncStmt& ast) -> Result;

    /** Generate IR for a RETURN statement. */
    [[nodiscard]] auto accept(const AstReturnStmt& ast) -> Result;

    /** Generate IR for a DIM statement. */
    [[nodiscard]] auto accept(const AstDimStmt& ast) -> Result;

    /** Generate IR for an assignment statement. */
    [[nodiscard]] auto accept(const AstAssignStmt& ast) -> Result;

    /** Generate IR for an IF statement. */
    [[nodiscard]] auto accept(const AstIfStmt& ast) -> Result;

    // -------------------------------------------------------------------------
    // Expressions (GenExpr.cpp)
    // -------------------------------------------------------------------------

    /** Generate IR for a cast expression. */
    [[nodiscard]] auto accept(AstCastExpr& ast) -> Result;

    /** Generate IR for a variable reference expression. */
    [[nodiscard]] static auto accept(AstVarExpr& ast) -> Result;

    /** Generate IR for a function/subroutine call expression. */
    [[nodiscard]] auto accept(AstCallExpr& ast) -> Result;

    /** Generate IR for a literal expression. */
    [[nodiscard]] auto accept(AstLiteralExpr& ast) const -> Result;

    /** Generate IR for a unary expression. */
    [[nodiscard]] auto accept(AstUnaryExpr& ast) -> Result;

    /** Generate IR for a binary expression. */
    [[nodiscard]] auto accept(AstBinaryExpr& ast) -> Result;

    /** Generate IR for a member access expression. */
    [[nodiscard]] auto accept(AstMemberExpr& ast) -> Result;

    // -------------------------------------------------------------------------
    // Types (GenType.cpp)
    // -------------------------------------------------------------------------

    /** Generate IR for a built-in type expression. */
    [[nodiscard]] auto accept(const AstBuiltInType& ast) -> Result;

    /** Generate IR for a pointer type expression. */
    [[nodiscard]] auto accept(const AstPointerType& ast) -> Result;

    /** Generate IR for a reference type expression. */
    [[nodiscard]] auto accept(const AstReferenceType& ast) -> Result;

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    /** Append an instruction to the current basic block's body. */
    void emit(lib::Instruction* instr) const;

    /** append terminating instruction only if current block is not terminated **/
    void terminate(lib::BasicBlock* target) const;

    /**
     * Create a new BasicBlock
     * Does NOT change the insertion point.
     */
    [[nodiscard]] auto createBlock(llvm::StringRef name) const -> lib::BasicBlock*;

    /**
     * Set given block as active
     */
    void setBlock(lib::BasicBlock* block);

    /** Check if the current block already ends with a terminator. */
    [[nodiscard]] auto isTerminated() const -> bool;

    /**
     * Create a numbered temporary value for expression results.
     * Numbering resets per function.
     */
    [[nodiscard]] auto createTemporary(const Type* type) -> lib::Temporary*;

    // -------------------------------------------------------------------------
    // Data
    // -------------------------------------------------------------------------
    lib::Module* m_module = nullptr;     ///< the IR module being generated
    lib::Function* m_function = nullptr; ///< current function
    lib::BasicBlock* m_block = nullptr;  ///< current insertion point
    unsigned m_tempCounter = 0;          ///< temporary numbering (resets per function)
    unsigned m_ifCounter = 0;            ///< if statement counter (resets per function)
};

} // namespace lbc::ir::gen
