//
// Created by Albert Varaksin on 05/07/2020.
//
#pragma once
#include "Ast.hpp"

namespace lbc {

template<class This,
        typename RetTy = void,
        typename StmtTy = RetTy,
        typename ExprTy = RetTy,
        typename TypeTy = RetTy>
class AstVisitor {
public:
    using GenRetTy = RetTy;
    using StmtRetTy = StmtTy;
    using ExprRetTy = ExprTy;
    using TypeRetTy = TypeTy;

#define VISIT_CASE(KIND) \
    case AstKind::KIND:  \
        return static_cast<This*>(this)->visit(static_cast<Ast##KIND&>(ast));

    StmtRetTy visit(AstStmt& ast) {
        switch (ast.kind) {
            AST_STMT_NODES(VISIT_CASE)
            AST_DECL_NODES(VISIT_CASE)
        default:
            llvm_unreachable(("visit: Unmatched Stmt: "_t + ast.getClassName()).str().c_str());
        }
    }

    ExprRetTy visit(AstExpr& ast) {
        switch (ast.kind) {
            AST_EXPR_NODES(VISIT_CASE)
        default:
            llvm_unreachable(("visit: Unmatched Expr: "_t + ast.getClassName()).str().c_str());
        }
    }
#undef VISIT_CASE
};

#define VISIT_METHOD_GEN(KIND) GenRetTy visit(Ast##KIND&);
#define VISIT_METHOD_EXPR(KIND) ExprRetTy visit(Ast##KIND&);
#define VISIT_METHOD_STMT(KIND) StmtRetTy visit(Ast##KIND&);
#define VISIT_METHOD_TYPE(KIND) TypeRetTy visit(Ast##KIND&);

#define AST_VISITOR_DECLARE_CONTENT_FUNCS() \
    using AstVisitor::visit;                \
    AST_BASIC_NODES(VISIT_METHOD_GEN)       \
    AST_STMT_NODES(VISIT_METHOD_STMT)       \
    AST_DECL_NODES(VISIT_METHOD_STMT)       \
    AST_TYPE_NODES(VISIT_METHOD_TYPE)       \
    AST_EXPR_NODES(VISIT_METHOD_EXPR)

} // namespace lbc
