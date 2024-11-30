//
// Created by Albert Varaksin on 05/07/2020.
//
#pragma once
#include "pch.hpp"
#include "Ast.hpp"

namespace lbc {

// NOLINTBEGIN(cppcoreguidelines-pro-type-static-cast-downcast)
template <class This, typename RetTy = void, typename StmtTy = RetTy, typename ExprTy = RetTy, typename TypeTy = RetTy>
class AstVisitor {
    NO_COPY_AND_MOVE(AstVisitor)

protected:
    AstVisitor() = default;
    virtual ~AstVisitor() = default;

public:
    using GenRetTy = RetTy;
    using StmtRetTy = StmtTy;
    using ExprRetTy = ExprTy;
    using TypeRetTy = TypeTy;

#define VISIT_CASE(KIND) \
    case AstKind::KIND:  \
        return static_cast<This*>(this)->visit(static_cast<Ast##KIND&>(ast));

    auto visit(AstStmt& ast) -> StmtRetTy {
        switch (ast.kind) {
            AST_STMT_NODES(VISIT_CASE)
            AST_DECL_NODES(VISIT_CASE)
        default:
            llvm_unreachable(("visit: Unmatched Stmt: "_t + ast.getClassName()).str().c_str());
        }
    }

    auto visit(AstExpr& ast) -> ExprRetTy {
        switch (ast.kind) {
            AST_EXPR_NODES(VISIT_CASE)
        default:
            llvm_unreachable(("visit: Unmatched Expr: "_t + ast.getClassName()).str().c_str());
        }
    }
#undef VISIT_CASE
};

template <class This, typename ExprTy = void>
class AstExprVisitor {
    NO_COPY_AND_MOVE(AstExprVisitor)
    AstExprVisitor() = default;
    virtual ~AstExprVisitor() = default;
    friend This;

public:
    using ExprRetTy = ExprTy;

    auto visit(AstExpr& ast) -> ExprRetTy {
        // clang-format off
        #define VISIT_CASE(KIND) \
            case AstKind::KIND:  \
            return static_cast<This*>(this)->visit(static_cast<Ast## KIND&>(ast));

        switch (ast.kind) {
            AST_EXPR_NODES(VISIT_CASE)
        default:
            llvm_unreachable(("visit: Unmatched Expr: "_t + ast.getClassName()).str().c_str());
        }
        #undef VISIT_CASE
        // clang-format on
    }
};

// NOLINTEND(cppcoreguidelines-pro-type-static-cast-downcast)

#define VISIT_METHOD_GEN(KIND) auto visit(Ast##KIND&) -> GenRetTy;
#define VISIT_METHOD_EXPR(KIND) auto visit(Ast##KIND&) -> ExprRetTy;
#define VISIT_METHOD_STMT(KIND) auto visit(Ast##KIND&) -> StmtRetTy;
#define VISIT_METHOD_TYPE(KIND) auto visit(Ast##KIND&) -> TypeRetTy;

#define AST_VISITOR_DECLARE_CONTENT_FUNCS() \
    using AstVisitor::visit;                \
    AST_BASIC_NODES(VISIT_METHOD_GEN)       \
    AST_STMT_NODES(VISIT_METHOD_STMT)       \
    AST_DECL_NODES(VISIT_METHOD_STMT)       \
    AST_TYPE_NODES(VISIT_METHOD_TYPE)       \
    AST_EXPR_NODES(VISIT_METHOD_EXPR)

#define AST_EXPR_VISITOR_DECLARE_CONTENT_FUNCS() \
    using AstExprVisitor::visit;                 \
    AST_EXPR_NODES(VISIT_METHOD_EXPR)

} // namespace lbc
