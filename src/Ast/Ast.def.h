//
// Created by Albert on 05/07/2020.
//
#pragma once

// clang-format off

#define AST_ROOT_NODES(_) \
    _( Root ) \
    _( Stmt ) \
    _( Expr ) \
    _( Decl )

/**
 * Statements
 */
#define AST_STMT_NODES(_) \
    _( Program    ) \
    _( StmtList   ) \
    _( AssignStmt ) \
    _( ExprStmt   )

// include declarations
#define AST_STMT_RANGE(_) _(Program, FuncParamDecl)

/**
 * Declarations
 */
#define AST_DECL_NODES(_) \
    _( VarDecl       ) \
    _( FuncDecl      ) \
    _( FuncParamDecl )

#define AST_DECL_RANGE(_) _(VarDecl, FuncParamDecl)

/**
 * Attributes
 */
#define AST_ATTRIB_NODES(_) \
    _( AttributeList ) \
    _( Attribute     )

#define AST_ATTR_RANGE(_) _(AttributeList, Attribute)

/**
 * Types
 */
#define AST_TYPE_NODES(_) \
    _( TypeExpr )

#define AST_TYPE_RANGE(_) _(TypeExpr, TypeExpr)

/**
 * Expressions
 */
#define AST_EXPR_NODES(_) \
    _( IdentExpr   ) \
    _( CallExpr    ) \
    _( LiteralExpr )

#define AST_EXPR_RANGE(_) _(IdentExpr, LiteralExpr)

/**
 * Combined
 */
#define AST_CONTENT_NODES(_) \
    AST_STMT_NODES(_)        \
    AST_DECL_NODES(_)        \
    AST_ATTRIB_NODES(_)      \
    AST_TYPE_NODES(_)        \
    AST_EXPR_NODES(_)

/**
 * Handy macro to forward declare all ast classes
 */
#define AST_FORWARD_DECLARE_IMPL(C) class Ast##C;
#define AST_FORWARD_DECLARE()           \
    AST_ROOT_NODES(AST_FORWARD_DECLARE_IMPL) \
    AST_CONTENT_NODES(AST_FORWARD_DECLARE_IMPL)
