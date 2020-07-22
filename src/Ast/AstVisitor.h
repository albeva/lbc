//
// Created by Albert on 05/07/2020.
//
#pragma once
#include "pch.h"
#include "Ast.h"

namespace lbc {

/**
 * Visit statement nodes
 *
 * @tparam ImplClass super class implementing this visitor
 * @tparam RetTy visit method return type
 */
template<typename ImplClass, typename RetTy = void>
class AstStmtVisitor {
public:
    using StmtReturnType = RetTy;

    RetTy visitStmt(AstStmt* ast) {
#define AST_VISITOR(KIND) \
    case AstKind::KIND:   \
        return static_cast<ImplClass*>(this)->visit##KIND(static_cast<Ast##KIND*>(ast));

        switch (ast->kind()) {
            AST_STMT_NODES(AST_VISITOR) // NOLINT
            AST_DECL_NODES(AST_VISITOR) // NOLINT
        default:
            llvm_unreachable("Invalid ast visit kind");
        }
#undef AST_VISITOR
    }
};

#define AST_DECLARE_STMT_VISIT_METHOD(KIND) StmtReturnType visit##KIND(Ast##KIND* ast);
#define AST_DECLARE_ALL_STMT_VISIT_METHODS() \
    AST_STMT_NODES(AST_DECLARE_STMT_VISIT_METHOD)

/**
 * Visit expression nodes
 *
 * @tparam ImplClass super class implementing this visitor
 * @tparam RetTy visit method return type
 */
template<typename ImplClass, typename RetTy = void>
class AstExprVisitor {
public:
    using ExprReturnType = RetTy;

    RetTy visitExpr(AstExpr* ast) {
#define AST_VISITOR(KIND) \
    case AstKind::KIND:   \
        return static_cast<ImplClass*>(this)->visit##KIND(static_cast<Ast##KIND*>(ast));

        switch (ast->kind()) {
            AST_EXPR_NODES(AST_VISITOR) // NOLINT
        default:
            llvm_unreachable("Invalid ast visit kind");
        }
#undef AST_VISITOR
    }
};

#define AST_DECLARE_EXPR_VISIT_METHOD(KIND) ExprReturnType visit##KIND(Ast##KIND* ast);
#define AST_DECLARE_ALL_EXPR_VISIT_METHODS() \
    AST_EXPR_NODES(AST_DECLARE_EXPR_VISIT_METHOD)

/**
 * Visit attribute nodes
 *
 * @tparam ImplClass super class implementing this visitor
 * @tparam RetTy visit method return type
 */
template<typename ImplClass, typename RetTy = void>
class AstAttrVisitor {
public:
    using AttrReturnType = RetTy;

    RetTy visitAttr(AstAttr* ast) {
#define AST_VISITOR(KIND) \
    case AstKind::KIND:   \
        return static_cast<ImplClass*>(this)->visit##KIND(static_cast<Ast##KIND*>(ast));

        switch (ast->kind()) {
            AST_ATTRIB_NODES(AST_VISITOR) // NOLINT
        default:
            llvm_unreachable("Invalid ast visit kind");
        }
#undef AST_VISITOR
    }
};

#define AST_DECLARE_ATTR_VISIT_METHOD(KIND) AttrReturnType visit##KIND(Ast##KIND* ast);
#define AST_DECLARE_ALL_ATTR_VISIT_METHODS() \
    AST_ATTRIB_NODES(AST_DECLARE_ATTR_VISIT_METHOD)

/**
 * Visit attribute nodes
 *
 * @tparam ImplClass super class implementing this visitor
 * @tparam RetTy visit method return type
 */
template<typename ImplClass, typename RetTy = void>
class AstTypeVisitor {
public:
    using TypeReturnType = RetTy;

    RetTy visitType(AstType* ast) {
#define AST_VISITOR(KIND) \
    case AstKind::KIND:   \
        return static_cast<ImplClass*>(this)->visit##KIND(static_cast<Ast##KIND*>(ast));

        switch (ast->kind()) {
            AST_TYPE_NODES(AST_VISITOR) // NOLINT
        default:
            llvm_unreachable("Invalid ast visit kind");
        }
#undef AST_VISITOR
    }
};

#define AST_DECLARE_TYPE_VISIT_METHOD(KIND) TypeReturnType visit##KIND(Ast##KIND* ast);
#define AST_DECLARE_ALL_TYPE_VISIT_METHODS() \
    AST_TYPE_NODES(AST_DECLARE_TYPE_VISIT_METHOD)

/**
 * Visit declaration nodes
 *
 * @tparam ImplClass super class implementing this visitor
 * @tparam RetTy visit method return type
 */
template<typename ImplClass, typename RetTy = void>
class AstDeclVisitor {
public:
    using DeclReturnType = RetTy;

    RetTy visitDecl(AstDecl* ast) {
#define AST_VISITOR(KIND) \
    case AstKind::KIND:   \
        return static_cast<ImplClass*>(this)->visit##KIND(static_cast<Ast##KIND*>(ast));

        switch (ast->kind()) {
            AST_DECL_NODES(AST_VISITOR) // NOLINT
        default:
            llvm_unreachable("Invalid ast visit kind");
        }
#undef AST_VISITOR
    }
};

#define AST_DECLARE_DECL_VISIT_METHOD(KIND) DeclReturnType visit##KIND(Ast##KIND* ast);
#define AST_DECLARE_ALL_DECL_VISIT_METHODS() \
    AST_DECL_NODES(AST_DECLARE_DECL_VISIT_METHOD)

/**
 * Super visitor for all nodes
 *
 * @tparam ImplClass super class implementing the visitor
 * @tparam RetTy return type for visit methods
 */
template<
    typename ImplClass,
    typename RetTy = void,
    typename StmtRetTy = RetTy,
    typename ExprRetTy = RetTy,
    typename AttrRetTy = RetTy,
    typename TypeRetTy = RetTy,
    typename DeclRetTy = RetTy>
class AstVisitor : public AstStmtVisitor<ImplClass, StmtRetTy>
, public AstExprVisitor<ImplClass, ExprRetTy>
, public AstAttrVisitor<ImplClass, AttrRetTy>
, public AstTypeVisitor<ImplClass, TypeRetTy>
, public AstDeclVisitor<ImplClass, DeclRetTy> {
public:
    using ReturnType = RetTy;
};

#define AST_DECLARE_ALL_ROOT_VISIT_METHODS() \
    AST_DECLARE_ALL_STMT_VISIT_METHODS()     \
    AST_DECLARE_ALL_EXPR_VISIT_METHODS()     \
    AST_DECLARE_ALL_ATTR_VISIT_METHODS()     \
    AST_DECLARE_ALL_TYPE_VISIT_METHODS()     \
    AST_DECLARE_ALL_DECL_VISIT_METHODS()

} // namespace lbc
