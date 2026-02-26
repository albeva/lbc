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
class SymbolTable;

/**
 * Analyse a child expression of an AST node and write the (possibly replaced)
 * pointer back. The expression may be wrapped in an implicit AstCastExpr if
 * type coercion is needed, so the parent must be updated with the new pointer.
 *
 * @param ast   Parent AST node that owns the expression child.
 * @param name  Suffix for get/set accessors (e.g. `Expr` → `getExpr()` / `setExpr()`).
 * @param type  Implicit target type for coercion, or `nullptr` if unconstrained.
 */
#define TRY_EXPRESSION(ast, name, type) \
    TRY_EXPRESSION_IMPL(TRY_RESULT(try_expr_), ast, name, type)
#define TRY_EXPRESSION_IMPL(id, ast, name, type)            \
    {                                                       \
        TRY_DECL(id, expression(*(ast).get##name(), type)); \
        (ast).set##name(id);                                \
    }

/**
 * Semantic analyser. Implementation is split across multiple
 * .cpp files by concern: SemaDecl, SemaExpr, SemaStmt,
 * SemaType, and Sema.cpp for common utilities.
 */
class SemanticAnalyser final : LogProvider, AstVisitor<DiagResult<void>> {
public:
    NO_COPY_AND_MOVE(SemanticAnalyser)

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

    [[nodiscard]] auto getTypeFactory() const -> TypeFactory& { return m_context.getTypeFactory(); }

    /** Analyse the module root node. */
    [[nodiscard]] auto accept(AstModule& ast) -> Result;

    // -------------------------------------------------------------------------
    // Declarations (SemaDecl.cpp)
    // -------------------------------------------------------------------------

    /**
     * Declare a new symbol from ast node
     */
    [[nodiscard]] auto declare(AstDecl& ast) -> Result;

    /**
     * Define symbol from the ast.
     */
    [[nodiscard]] auto define(AstDecl& ast) -> Result;

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
    [[nodiscard]] static auto accept(AstDeclareStmt& ast) -> Result;

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

    /**
     * Analyse an expression, applying implicit type coercion if needed.
     *
     * Saves and restores m_implicitType / m_suggestedType via ValueRestorer.
     * After the expression visitor runs, if the result type differs from
     * @p implicitType the expression is wrapped in an implicit cast.
     *
     * @param ast          The expression AST node to analyse.
     * @param explicitType Target type for coercion, or nullptr if unconstrained.
     * @return The original or replaced (cast-wrapped) expression pointer.
     */
    [[nodiscard]] auto expression(AstExpr& ast, const Type* explicitType) -> DiagResult<AstExpr*>;

    /** Analyse an explicit or implicit cast expression. */
    [[nodiscard]] auto accept(AstCastExpr& ast) -> Result;

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

    /**
     * Insert an implicit cast if the expression type is convertible to the target.
     * Returns the expression unchanged if types already match, or a new
     * AstCastExpr wrapping it. Diagnoses incompatible types.
     */
    [[nodiscard]] auto coerce(AstExpr& ast, const Type* targetType) -> DiagResult<AstExpr*>;

    /**
     * Try literal coercion first, then fall back to coerce().
     * For literals this may simply re-type the node (no cast node needed);
     * for non-literals it delegates to coerce() which inserts an implicit cast.
     */
    [[nodiscard]] auto castOrCoerce(AstExpr& ast, const Type* targetType) -> DiagResult<AstExpr*>;

    /**
     * Re-type a literal to match the target type within the same type family.
     * Integral literals can adopt any integral type, float literals any float
     * type, and null literals any pointer type. Cross-family coercion is rejected.
     */
    [[nodiscard]] auto coerceLiteral(AstLiteralExpr& ast, const Type* targetType) -> Result;

    /**
     * Record a type suggestion that propagates upward through the expression tree.
     * Set by any typed sub-expression (variables, literals, casts, calls). When
     * multiple suggestions compete, their common type is used. Guides literal
     * coercion in parent binary expressions (e.g. `2 + b` where b is BYTE).
     */
    void setSuggestedType(const Type* implicitType);

    /**
     * Verify that an expression is addressable (can have its address taken).
     * Currently only variable references are addressable.
     */
    [[nodiscard]] auto ensureAddressable(AstExpr& ast) -> Result;

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
    SymbolTable* m_symbolTable = nullptr;

    /// Target type pushed down from the caller (e.g. `DIM x AS BYTE = <expr>`).
    /// Literals adopt this type if compatible; non-literals are coerced after visit.
    const Type* m_explicitType = nullptr;

    /// Type that propagates upward from typed sub-expressions (variables, casts,
    /// calls, literals). Guides literal coercion in binary expressions: `2 + b`
    /// where b is BYTE suggests BYTE, coercing the literal 2 to match.
    /// When multiple suggestions compete, their common type is used.
    const Type* m_suggestedType = nullptr;
};

} // namespace lbc
