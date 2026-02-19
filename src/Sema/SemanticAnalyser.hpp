//
// Created by Albert Varaksin on 19/02/2026.
//
#pragma once
#include "pch.hpp"

#include "Ast/Ast.hpp"
#include "Ast/AstVisitor.hpp"
#include "Diag/DiagEngine.hpp"
#include "Diag/LogProvider.hpp"
namespace lbc {
class Context;

/**
 * Semantic analyser. Implementation is split across multiple
 * .cpp files by concern: SemaDecl, SemaExpr, SemaStmt,
 * SemaType, and Sema.cpp for common utilities.
 */
class SemanticAnalyser final : LogProvider, AstVisitor<DiagResult<void>> {
public:
    NO_COPY_AND_MOVE(SemanticAnalyser)
    using Result = DiagResult<void>;

    /**
     * Construct a semantic analyser for the given context.
     */
    explicit SemanticAnalyser(Context& context);
    ~SemanticAnalyser() final;

    /**
     * Run semantic analysis on the given AST module.
     */
    [[nodiscard]] auto analyse(AstModule& ast) -> Result;

    /**
     * Get associated context object.
     */
    [[nodiscard]] auto getContext() -> Context& { return m_context; }

private:
    friend AstVisitor;

    /** Analyse the module root node. */
    [[nodiscard]] auto accept(AstModule& ast) -> Result;

    // -------------------------------------------------------------------------
    // Declarations (SemaDecl.cpp)
    // -------------------------------------------------------------------------

    /** Analyse a variable declaration. */
    [[nodiscard]] auto accept(AstVarDecl& ast) -> Result;

    /** Analyse a function declaration. */
    [[nodiscard]] auto accept(AstFuncDecl& ast) -> Result;

    /** Analyse a function parameter declaration. */
    [[nodiscard]] auto accept(AstFuncParamDecl& ast) -> Result;

    // -------------------------------------------------------------------------
    // Statements (SemaStmt.cpp)
    // -------------------------------------------------------------------------

    /** Analyse a statement list. */
    [[nodiscard]] auto accept(AstStmtList& ast) -> Result;

    /** Analyse an expression statement. */
    [[nodiscard]] auto accept(AstExprStmt& ast) -> Result;

    /** Analyse a DECLARE statement. */
    [[nodiscard]] auto accept(AstDeclareStmt& ast) -> Result;

    /** Analyse a function body statement. */
    [[nodiscard]] auto accept(AstFuncStmt& ast) -> Result;

    /** Analyse a RETURN statement. */
    [[nodiscard]] auto accept(AstReturnStmt& ast) -> Result;

    /** Analyse a DIM statement. */
    [[nodiscard]] auto accept(AstDimStmt& ast) -> Result;

    /** Analyse an assignment statement. */
    [[nodiscard]] auto accept(AstAssignStmt& ast) -> Result;

    /** Analyse an IF statement. */
    [[nodiscard]] auto accept(AstIfStmt& ast) -> Result;

    // -------------------------------------------------------------------------
    // Expressions (SemaExpr.cpp)
    // -------------------------------------------------------------------------

    /** Analyse a variable reference expression. */
    [[nodiscard]] auto accept(AstVarExpr& ast) -> Result;

    /** Analyse a function/subroutine call expression. */
    [[nodiscard]] auto accept(AstCallExpr& ast) -> Result;

    /** Analyse a literal expression. */
    [[nodiscard]] auto accept(AstLiteralExpr& ast) -> Result;

    /** Analyse a unary expression. */
    [[nodiscard]] auto accept(AstUnaryExpr& ast) -> Result;

    /** Analyse a binary expression. */
    [[nodiscard]] auto accept(AstBinaryExpr& ast) -> Result;

    /** Analyse a member access expression. */
    [[nodiscard]] auto accept(AstMemberExpr& ast) -> Result;

    // -------------------------------------------------------------------------
    // Types (SemaType.cpp)
    // -------------------------------------------------------------------------

    /** Analyse a built-in type expression. */
    [[nodiscard]] auto accept(AstBuiltInType& ast) -> Result;

    /** Analyse a pointer type expression. */
    [[nodiscard]] auto accept(AstPointerType& ast) -> Result;

    /** Analyse a reference type expression. */
    [[nodiscard]] auto accept(AstReferenceType& ast) -> Result;

    // -------------------------------------------------------------------------
    // Data
    // -------------------------------------------------------------------------
    Context& m_context;
};

} // namespace lbc
