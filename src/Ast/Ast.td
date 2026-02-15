// AST node definitions for lbc
// Processed by lbc-tblgen to generate Ast.hpp and AstVisitor.hpp

// -----------------------------------------------------------------------------
// Arg field on a node.
// -----------------------------------------------------------------------------
class Arg<string type_, string name_, bit mutable_ = false, string default_ = ""> {
    string type = type_;
    string name = name_;
    bit mutable = mutable_;
    string default = default_;
}

// Forward declare Group
class Group;

// -----------------------------------------------------------------------------
// Base AST node. All nodes carry a source range.
// -----------------------------------------------------------------------------
class Node<string desc_ = "", Group parent_, list<Arg> args_ = []> {
    string desc = desc_;
    Group parent = parent_;
    list<Arg> args = args_;
}

// -----------------------------------------------------------------------------
// Group node — an abstract intermediate that concrete nodes inherit from.
// Groups define shared members and form the parent in the hierarchy.
// -----------------------------------------------------------------------------
class Group<
    string desc_ = "",
    Group parent_,
    list<Arg> args_ = []
> : Node<desc_, parent_, args_>;

// -----------------------------------------------------------------------------
// Leaf node — a concrete, instantiable AST node belonging to a parent.
// -----------------------------------------------------------------------------
class Leaf<
    string desc_ = "",
    Group parent_,
    list<Arg> args_ = []
> : Node<desc_, parent_, args_>;

// ============================================================================
// Root
// ============================================================================

def Root : Group<"The root AST node", ?, [
    Arg<"llvm::SMRange", "range">,
    Arg<"AstRoot*", "next", true, "nullptr">
]>;

def Module : Leaf<"Program module node", Root, [
    Arg<"AstStmtList*", "stmtList">
]>;

// ============================================================================
// Types
// ============================================================================

def Type : Group<"type expression", Root, [
    Arg<"const Type*", "type", true, "nullptr">
]>;
def BuiltInType   : Leaf<"Built-in primitive type", Type, [
    Arg<"TokenKind", "tokenKind">
]>;

// ============================================================================
// Statements
// ============================================================================

def Stmt : Group<"statement", Root>;

def StmtList : Leaf<"List of statements", Stmt, [
    Arg<"std::span<AstStmt*>", "stmts">,
    Arg<"std::span<AstDecl*>", "decls">
]>;

def ExprStmt : Leaf<"Expression statement", Stmt, [
    Arg<"AstExpr*", "expr">
]>;

def DeclareStmt : Leaf<"Forward declaration statement", Stmt, [
    Arg<"AstFuncDecl*", "decl">
]>;

def FuncStmt : Leaf<"Function or subroutine definition", Stmt, [
    Arg<"AstFuncDecl*", "decl">,
    Arg<"AstStmtList*", "stmtList">
]>;

def ReturnStmt : Leaf<"Return statement", Stmt, [
    Arg<"AstExpr*", "expr">
]>;

def DimStmt : Leaf<"DIM variable declaration statement", Stmt, [
    Arg<"std::span<AstVarDecl*>", "decls">
]>;

def AssignStmt : Leaf<"Assignment statement", Stmt, [
    Arg<"AstExpr*", "assignee">,
    Arg<"AstExpr*", "expr">
]>;

def IfStmt : Leaf<"IF statement", Stmt, [
    Arg<"AstExpr*", "condition">,
    Arg<"AstStmt*", "thenStmt">,
    Arg<"AstStmt*", "elseStmt">
]>;

// ============================================================================
// Declarations
// ============================================================================

def Decl : Group<"declaration", Root, [
    Arg<"llvm::StringRef", "name">,
    Arg<"const Type*", "type", true, "nullptr">
]>;

def VarDecl : Leaf<"Variable declaration", Decl, [
    Arg<"AstType*", "typeExpr">,
    Arg<"AstExpr*", "expr">
]>;

def FuncDecl : Leaf<"Function or subroutine declaration", Decl, [
    Arg<"std::span<AstFuncParamDecl*>", "params">,
    Arg<"AstType*", "retTypeExpr">,
    Arg<"AstFuncStmt*", "impl", true, "nullptr">
]>;

def FuncParamDecl : Leaf<"Function parameter declaration", Decl, [
    Arg<"AstType*", "typeExpr">
]>;

// ============================================================================
// Expressions
// ============================================================================

def Expr : Group<"expression", Root, [
    Arg<"const Type*", "type", true, "nullptr">
]>;

def VarExpr : Leaf<"Variable expression", Expr, [
    Arg<"llvm::StringRef", "name">,
]>;

def CallExpr : Leaf<"CALL expression", Expr, [
    Arg<"AstExpr*", "callee">,
    Arg<"std::span<AstExpr*>", "args">,
]>;

def LiteralExpr : Leaf<"Literal expression", Expr, [
    Arg<"LiteralValue", "value">
]>;

def UnaryExpr : Leaf<"Unary expression", Expr, [
    Arg<"AstExpr*", "expr">,
    Arg<"TokenKind", "op">,
]>;

def BinaryExpr : Leaf<"Binary expression", Expr, [
    Arg<"AstExpr*", "left">,
    Arg<"AstExpr*", "right">,
    Arg<"TokenKind", "op">,
]>;
