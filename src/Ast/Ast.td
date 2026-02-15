// AST node definitions for lbc
// Processed by lbc-tblgen to generate Ast.hpp and AstVisitor.hpp

// Member field on a node.
class Member<string type_, string name_, bit mutable_ = false, string default_ = ""> {
    string type = type_;
    string name = name_;
    bit mutable = mutable_;
    string default = default_;
}

// Forward declare Group
class Group;

// Base AST node. All nodes carry a source range.
class Node<string desc_ = "", Group parent_, list<Member> members_ = []> {
    string desc = desc_;
    Group parent = parent_;
    list<Member> members = members_;
}

// Group node — an abstract intermediate that concrete nodes inherit from.
// Groups define shared members and form the parent in the hierarchy.
class Group<
    string desc_ = "",
    Group parent_,
    list<Member> members_ = []
> : Node<desc_, parent_, members_>;

// Leaf node — a concrete, instantiable AST node belonging to a parent.
class Leaf<
    string desc_ = "",
    Group parent_,
    list<Member> members_ = []
> : Node<desc_, parent_, members_>;

// ============================================================================
// Root
// ============================================================================

def Root : Group<"The root AST node", ?, [
    Member<"llvm::SMRange", "range">,
    Member<"AstRoot*", "next", true, "nullptr">
]>;

// ============================================================================
// Module
// ============================================================================

def Module : Leaf<"Program module node", Root, [
    Member<"std::span<AstImportDecl*>", "imports">,
    Member<"AstStmtList*", "stmtList">,
//    Member<"SymbolTable*", "symbolTable", false, "nullptr">,
    Member<"bool", "implicitMain", true, "false">,
]>;

// ============================================================================
// Types
// ============================================================================

def Type : Group<"type expression", Root, [
    Member<"const Type*", "type", true, "nullptr">
]>;
def BuiltInType   : Leaf<"Built-in primitive type", Type, [
    Member<"TokenKind", "tokenKind">
]>;
def PointerType   : Leaf<"Pointer type", Type, [
    Member<"AstType*", "typeExpr">
]>;
def ReferenceType : Leaf<"Reference type", Type, [
    Member<"AstType*", "typeExpr">
]>;

// ============================================================================
// Statements
// ============================================================================

def Stmt : Group<"statement", Root>;

def StmtList : Leaf<"List of statements", Stmt, [
    Member<"std::span<AstStmt*>", "stmts">,
    Member<"std::span<AstDecl*>", "decls">,
//    Member<"SymbolTable*", "symbolTable", false, "nullptr">
]>;

def EmptyStmt : Leaf<"Empty statement", Stmt>;

def DimStmt : Leaf<"DIM variable declaration statement", Stmt, [
    Member<"std::span<AstVarDecl*>", "decls">
]>;

def ExprStmt : Leaf<"Expression statement", Stmt, [
    Member<"AstExpr*", "expr">
]>;

def DeclareStmt : Leaf<"Forward declaration statement", Stmt, [
    Member<"AstFuncDecl*", "decl">
]>;

def ExternStmt : Leaf<"External linkage block", Stmt, [
    Member<"AstStmtList*", "stmtList">,
//    Member<"CallingConvention", "callingConv">,
]>;

def AssignStmt : Leaf<"Assignment statement", Stmt, [
    Member<"AstExpr*", "assignee">,
    Member<"AstExpr*", "expr">,
]>;

def FuncStmt : Leaf<"Function or subroutine definition", Stmt, [
    Member<"AstFuncDecl*", "decl">,
    Member<"AstStmtList*", "stmtList">,
]>;

def ReturnStmt : Leaf<"Return statement", Stmt, [
    Member<"AstExpr*", "expr">
]>;

def IfStmt : Leaf<"IF statement", Stmt, [
    Member<"AstExpr*", "condition">,
    Member<"AstStmt*", "thenStmt">,
    Member<"AstStmt*", "elseStmt">,
]>;

// ============================================================================
// Declarations
// ============================================================================

def Decl : Group<"declaration", Root, [
    Member<"llvm::StringRef", "name">,
//    Member<"CallingConvention", "callingConv">,
//    Member<"SymbolVisibility", "visibility">,
    Member<"const Type*", "type", true, "nullptr">,
//    Member<"Symbol*", "symbol", true, "nullptr">,
]>;

def ImportDecl : Leaf<"Import declaration", Decl>;

def VarDecl : Leaf<"Variable declaration", Decl, [
    Member<"AstType*", "typeExpr">,
    Member<"AstExpr*", "expr">,
]>;

def FuncDecl : Leaf<"Function or subroutine declaration", Decl, [
    Member<"std::span<AstFuncParamDecl*>", "params">,
    Member<"AstType*", "retTypeExpr">,
    Member<"AstFuncStmt*", "impl", true, "nullptr">,
]>;

def FuncParamDecl : Leaf<"Function parameter declaration", Decl, [
    Member<"AstType*", "typeExpr">,
    Member<"bool", "isVarArg">,
]>;

// ============================================================================
// Expressions
// ============================================================================

def Expr : Group<"expression", Root, [
    Member<"const Type*", "type", true, "nullptr">
]>;

def VariableExpr : Leaf<"Variable expression", Expr, [
    Member<"llvm::StringRef", "name">,
//    Member<"Symbol*", "symbol", true, "nullptr">,
]>;

def CallExpr : Leaf<"CALL expression", Expr, [
    Member<"AstExpr*", "callee">,
    Member<"std::span<AstExpr*>", "args">,
]>;

def LiteralExpr : Leaf<"Literal expression", Expr, [
    Member<"LiteralValue", "value">
]>;

def UnaryExpr : Leaf<"Unary expression", Expr, [
    Member<"AstExpr*", "expr">,
    Member<"TokenKind", "tokenKind">,
]>;

def BinaryExpr : Leaf<"Binary expression", Expr, [
    Member<"AstExpr*", "left">,
    Member<"AstExpr*", "right">,
    Member<"TokenKind", "tokenKind">,
]>;

def CastExpr : Leaf<"CAST expression", Expr, [
    Member<"AstExpr*", "expr">,
    Member<"AstType*", "typeExpr">,
    Member<"bool", "isImplicit">,
]>;

def DereferenceExpr : Leaf<"DEREFERENCE expression", Expr, [
    Member<"AstExpr*", "expr">
]>;

def AddressOfExpr : Leaf<"ADDRESS OF expression", Expr, [
    Member<"AstExpr*", "expr">
]>;

def MemberExpr : Leaf<"Member expression", Expr, [
    Member<"AstExpr*", "expr">,
    Member<"AstExpr*", "member">,
    Member<"TokenKind", "tokenKind">,
]>;
