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
namespace lbc::ir::gen {

/**
 * IR generator. Lowers the analysed AST into IR instructions.
 *
 * Implementation is split across multiple .cpp files by concern:
 * GenDecl, GenExpr, GenStmt, GenType, and Gen.cpp for common utilities.
 */
class IrGenerator final : LogProvider, AstVisitor<DiagResult<void>>, lib::Builder {
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
    [[nodiscard]] auto generate(const AstModule& ast) -> Result;

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
    [[nodiscard]] auto accept(const AstDeclareStmt& ast) -> Result;

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
    [[nodiscard]] auto accept(const AstCastExpr& ast) -> Result;

    /** Generate IR for a variable reference expression. */
    [[nodiscard]] auto accept(const AstVarExpr& ast) -> Result;

    /** Generate IR for a function/subroutine call expression. */
    [[nodiscard]] auto accept(const AstCallExpr& ast) -> Result;

    /** Generate IR for a literal expression. */
    [[nodiscard]] auto accept(const AstLiteralExpr& ast) -> Result;

    /** Generate IR for a unary expression. */
    [[nodiscard]] auto accept(const AstUnaryExpr& ast) -> Result;

    /** Generate IR for a binary expression. */
    [[nodiscard]] auto accept(const AstBinaryExpr& ast) -> Result;

    /** Generate IR for a member access expression. */
    [[nodiscard]] auto accept(const AstMemberExpr& ast) -> Result;

    // -------------------------------------------------------------------------
    // Types (GenType.cpp)
    // -------------------------------------------------------------------------

    /** Generate IR for a built-in type expression. */
    [[nodiscard]] auto accept(const AstBuiltInType& ast) -> Result;

    /** Generate IR for a pointer type expression. */
    [[nodiscard]] auto accept(const AstPointerType& ast) -> Result;

    /** Generate IR for a reference type expression. */
    [[nodiscard]] auto accept(const AstReferenceType& ast) -> Result;
};

} // namespace lbc::ir::gen
