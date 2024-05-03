//
// Created by Albert Varaksin on 05/07/2020.
//
#pragma once
#include "pch.hpp"
#include "Ast.def.hpp"
#include "ControlFlowStack.hpp"
#include "Lexer/Token.hpp"
#include "Symbol/SymbolTable.hpp"
#include "ValueFlags.hpp"

namespace lbc {
class TypeRoot;
AST_FORWARD_DECLARE()

enum class CallingConv {
    Default,
    C
};

enum class AstKind {
#define KIND_ENUM(id, ...) id,
    AST_CONTENT_NODES(KIND_ENUM)
#undef KIND_ENUM
        Extern
};

/**
 * Root class for all AST nodes. This is an abstract class
 * and should never be used as type for ast node directly
 */
struct AstRoot {
    NO_COPY_AND_MOVE(AstRoot)

    constexpr AstRoot(AstKind kind_, llvm::SMRange range_)
    : kind{ kind_ }, range{ range_ } {}

    virtual ~AstRoot() = default;

    [[nodiscard]] llvm::StringRef getClassName() const;

    const AstKind kind;
    const llvm::SMRange range;

    // Make vanilla new/delete illegal.
    void* operator new(size_t) = delete;
    void operator delete(void* /* ptr */) {} // NOLINT(cert-dcl54-cpp,hicpp-new-delete-operators,misc-new-delete-overloads)

    // Allow placement new
    void* operator new(size_t /*size*/, void* ptr) {
        assert(ptr); // NOLINT
        return ptr;
    }
};

#define IS_AST_CLASSOF(FIRST, LAST) ast->kind >= AstKind::FIRST && ast->kind <= AstKind::LAST;

//----------------------------------------
// Module
//----------------------------------------

struct AstModule final : AstRoot {
    AstModule(
        unsigned int file,
        llvm::SMRange range_,
        bool implicitMain,
        std::vector<AstImport*>&& imports_,
        AstStmtList* stms)
    : AstRoot{ AstKind::Module, range_ },
      fileId{ file },
      hasImplicitMain{ implicitMain },
      imports{ std::move(imports_) },
      stmtList{ stms } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::Module;
    }

    const unsigned int fileId;
    const bool hasImplicitMain;
    std::vector<AstImport*> imports;
    AstStmtList* stmtList;
    SymbolTable* symbolTable = nullptr;
};

//----------------------------------------
// Statements
//----------------------------------------

struct AstStmt : AstRoot {
    using AstRoot::AstRoot;

    static constexpr bool classof(const AstRoot* ast) {
        return AST_STMT_RANGE(IS_AST_CLASSOF)
    }
};

struct AstStmtList final : AstStmt {
    AstStmtList(
        llvm::SMRange range_,
        std::vector<AstDecl*>&& decl_,
        std::vector<AstFuncStmt*>&& funcs_,
        std::vector<AstStmt*>&& stmts_)
    : AstStmt{ AstKind::StmtList, range_ },
      decl{ std::move(decl_) },
      funcs{ std::move(funcs_) },
      stmts{ std::move(stmts_) } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::StmtList;
    }

    std::vector<AstDecl*> decl;
    std::vector<AstFuncStmt*> funcs;
    std::vector<AstStmt*> stmts;
};

struct AstImport final : AstStmt {
    AstImport(
        llvm::SMRange range_,
        llvm::StringRef import_,
        AstModule* module_ = nullptr)
    : AstStmt{ AstKind::Import, range_ },
      import{ import_ },
      module{ module_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::Import;
    }

    const llvm::StringRef import;
    AstModule* module;
};

struct AstExtern final : AstStmt {
    AstExtern(
        llvm::SMRange range_,
        CallingConv language_,
        std::vector<AstStmt*>&& stmts_)
    : AstStmt{ AstKind::Extern, range_ },
      langauge{ language_ },
      stmts{ std::move(stmts_) } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::Extern;
    }

    CallingConv langauge;
    std::vector<AstStmt*> stmts;
};

struct AstExprStmt final : AstStmt {
    AstExprStmt(
        llvm::SMRange range_,
        AstExpr* expr_)
    : AstStmt{ AstKind::ExprStmt, range_ },
      expr{ expr_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::ExprStmt;
    }

    AstExpr* expr;
};

struct AstFuncStmt final : AstStmt {
    AstFuncStmt(
        llvm::SMRange range_,
        AstFuncDecl* decl_,
        AstStmtList* stmtList_)
    : AstStmt{ AstKind::FuncStmt, range_ },
      decl{ decl_ },
      stmtList{ stmtList_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::FuncStmt;
    }

    AstFuncDecl* decl;
    AstStmtList* stmtList;
};

struct AstReturnStmt final : AstStmt {
    AstReturnStmt(
        llvm::SMRange range_,
        AstExpr* expr_)
    : AstStmt{ AstKind::ReturnStmt, range_ },
      expr{ expr_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::ReturnStmt;
    }

    AstExpr* expr;
};

struct AstIfStmtBlock final {
    std::vector<AstVarDecl*> decls;
    SymbolTable* symbolTable;
    AstExpr* expr;
    AstStmt* stmt;

    AstIfStmtBlock(
        std::vector<AstVarDecl*>&& decls_,
        SymbolTable* symbolTable_,
        AstExpr* expr_,
        AstStmt* stmt_)
    : decls{ std::move(decls_) },
      symbolTable{ symbolTable_ },
      expr{ expr_ },
      stmt{ stmt_ } {}
};

struct AstIfStmt final : AstStmt {
    AstIfStmt(
        llvm::SMRange range_,
        std::vector<AstIfStmtBlock*>&& blocks_)
    : AstStmt{ AstKind::IfStmt, range_ },
      blocks{ std::move(blocks_) } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::IfStmt;
    }

    std::vector<AstIfStmtBlock*> blocks;
};

struct AstForStmt final : AstStmt {
    enum class Direction {
        Unknown,
        Skip,
        Increment,
        Decrement
    };

    AstForStmt(
        llvm::SMRange range_,
        std::vector<AstVarDecl*>&& decls_,
        AstVarDecl* iter_,
        AstExpr* limit_,
        AstExpr* step_,
        AstStmt* stmt_,
        llvm::StringRef next_)
    : AstStmt{ AstKind::ForStmt, range_ },
      decls{ std::move(decls_) },
      iterator{ iter_ },
      limit{ limit_ },
      step{ step_ },
      stmt{ stmt_ },
      next{ next_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::ForStmt;
    }

    std::vector<AstVarDecl*> decls;
    AstVarDecl* iterator;
    AstExpr* limit;
    AstExpr* step;
    AstStmt* stmt;
    const llvm::StringRef next;

    Direction direction = Direction::Unknown;
    SymbolTable* symbolTable = nullptr;
};

struct AstDoLoopStmt final : AstStmt {
    enum class Condition {
        None,
        PreWhile,
        PreUntil,
        PostWhile,
        PostUntil
    };

    AstDoLoopStmt(
        llvm::SMRange range_,
        std::vector<AstVarDecl*>&& decls_,
        Condition condition_,
        AstExpr* expr_,
        AstStmt* stmt_)
    : AstStmt{ AstKind::DoLoopStmt, range_ },
      decls{ std::move(decls_) },
      condition{ condition_ },
      expr{ expr_ },
      stmt{ stmt_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::DoLoopStmt;
    }

    std::vector<AstVarDecl*> decls;
    const Condition condition;
    AstExpr* expr;
    AstStmt* stmt;
    SymbolTable* symbolTable = nullptr;
};

enum class AstContinuationAction {
    Continue,
    Exit
};

struct AstContinuationStmt final : AstStmt {
    explicit AstContinuationStmt(
        llvm::SMRange range_,
        AstContinuationAction action_,
        std::size_t destination_)
    : AstStmt{ AstKind::ContinuationStmt, range_ },
      action{ action_ },
      destination{ std::move(destination_) } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::ContinuationStmt;
    }

    AstContinuationAction action;
    std::size_t destination;
};

//----------------------------------------
// Attributes
//----------------------------------------

struct AstAttributeList final : AstRoot {
    AstAttributeList(
        llvm::SMRange range_,
        std::vector<AstAttribute*>&& attribs_)
    : AstRoot{ AstKind::AttributeList, range_ },
      attribs{ std::move(attribs_) } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::AttributeList;
    }

    [[nodiscard]] bool exists(llvm::StringRef name) const;
    [[nodiscard]] std::optional<llvm::StringRef> getStringLiteral(llvm::StringRef key) const;

    std::vector<AstAttribute*> attribs;
};

struct AstAttribute final : AstRoot {
    AstAttribute(
        llvm::SMRange range_,
        AstIdentExpr* ident,
        AstExprList* args_)
    : AstRoot{ AstKind::Attribute, range_ },
      identExpr{ ident },
      args{ args_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::Attribute;
    }

    AstIdentExpr* identExpr;
    AstExprList* args;
};

//----------------------------------------
// Declarations
//----------------------------------------

struct AstDecl : AstStmt {
    AstDecl(
        AstKind kind_,
        llvm::SMRange range_,
        llvm::StringRef name_,
        Token token_,
        CallingConv callingConv_,
        AstAttributeList* attribs)
    : AstStmt{ kind_, range_ },
      name{ name_ },
      token{ token_ },
      callingConv{ callingConv_ },
      attributes{ attribs } {}

    static constexpr bool classof(const AstRoot* ast) {
        return AST_DECL_RANGE(IS_AST_CLASSOF)
    }

    const llvm::StringRef name;
    Token token;
    CallingConv callingConv;
    AstAttributeList* attributes;
    bool local = true;
    Symbol* symbol = nullptr;
};

struct AstDeclList final : AstRoot {
    AstDeclList(llvm::SMRange range_, std::vector<AstDecl*>&& decls_)
    : AstRoot{ AstKind::DeclList, range_ },
      decls{ std::move(decls_) } {}

    std::vector<AstDecl*> decls;
};

struct AstVarDecl final : AstDecl {
    AstVarDecl(
        llvm::SMRange range_,
        llvm::StringRef name_,
        Token token_,
        CallingConv callingConv_,
        AstAttributeList* attrs_,
        AstTypeExpr* type_,
        AstExpr* expr_)
    : AstDecl{ AstKind::VarDecl, range_, name_, token_, callingConv_, attrs_ },
      typeExpr{ type_ },
      expr{ expr_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::VarDecl;
    }

    AstTypeExpr* typeExpr;
    AstExpr* expr;
};

struct AstFuncDecl final : AstDecl {
    AstFuncDecl(
        llvm::SMRange range_,
        llvm::StringRef name_,
        Token token_,
        CallingConv callingConv_,
        AstAttributeList* attrs_,
        AstFuncParamList* params_,
        bool variadic_,
        AstTypeExpr* retType_,
        bool hasImpl_)
    : AstDecl{ AstKind::FuncDecl, range_, name_, token_, callingConv_, attrs_ },
      params{ params_ },
      variadic{ variadic_ },
      retTypeExpr{ retType_ },
      hasImpl{ hasImpl_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::FuncDecl;
    }

    AstFuncParamList* params;
    const bool variadic;
    AstTypeExpr* retTypeExpr;
    const bool hasImpl;
    SymbolTable* symbolTable = nullptr;
};

struct AstFuncParamDecl final : AstDecl {
    AstFuncParamDecl(
        llvm::SMRange range_,
        llvm::StringRef name_,
        Token token_,
        CallingConv callingConv_,
        AstAttributeList* attrs,
        AstTypeExpr* type_)
    : AstDecl{ AstKind::FuncParamDecl, range_, name_, token_, callingConv_, attrs },
      typeExpr{ type_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::FuncParamDecl;
    }

    AstTypeExpr* typeExpr;
};

struct AstFuncParamList final : AstRoot {
    AstFuncParamList(
        llvm::SMRange range_,
        std::vector<AstFuncParamDecl*>&& params_)
    : AstRoot{ AstKind::FuncParamList, range_ },
      params{ std::move(params_) } {}

    std::vector<AstFuncParamDecl*> params;
};

struct AstUdtDecl final : AstDecl {
    AstUdtDecl(
        llvm::SMRange range_,
        llvm::StringRef name_,
        Token token_,
        CallingConv callingConv_,
        AstAttributeList* attrs,
        AstDeclList* decls_)
    : AstDecl{ AstKind::UdtDecl, range_, name_, token_, callingConv_, attrs },
      decls{ decls_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::UdtDecl;
    }

    AstDeclList* decls;
    SymbolTable* symbolTable = nullptr;
};

struct AstTypeAlias final : AstDecl {
    AstTypeAlias(
        llvm::SMRange range_,
        llvm::StringRef name_,
        Token token_,
        CallingConv callingConv_,
        AstAttributeList* attrs,
        AstTypeExpr* typeExpr_)
    : AstDecl{ AstKind::TypeAlias, range_, name_, token_, callingConv_, attrs },
      typeExpr{ typeExpr_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::TypeAlias;
    }

    AstTypeExpr* typeExpr;
};

//----------------------------------------
// Types
//----------------------------------------
struct AstTypeOf final : AstRoot {
    using TypeExpr = std::variant<llvm::SMRange, AstTypeExpr*, AstExpr*>;

    explicit AstTypeOf(
        llvm::SMRange range_,
        TypeExpr&& typeExpr_)
    : AstRoot{ AstKind::TypeOf, range_ }, typeExpr{ typeExpr_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::TypeOf;
    }

    TypeExpr typeExpr;
    const TypeRoot* type = nullptr;
};

struct AstTypeExpr final : AstRoot {
    using TypeExpr = std::variant<AstIdentExpr*, AstFuncDecl*, AstTypeOf*, TokenKind>;

    AstTypeExpr(
        llvm::SMRange range_,
        TypeExpr expr_,
        int deref)
    : AstRoot{ AstKind::TypeExpr, range_ },
      expr{ expr_ },
      dereference{ deref } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::TypeExpr;
    }

    TypeExpr expr;
    const int dereference;
    const TypeRoot* type = nullptr;
};

//----------------------------------------
// Expressions
//----------------------------------------
struct AstExpr : AstRoot {
    using AstRoot::AstRoot;

    static constexpr bool classof(const AstRoot* ast) {
        return AST_EXPR_RANGE(IS_AST_CLASSOF)
    }

    const TypeRoot* type = nullptr;
    ValueFlags flags{};
};

struct AstExprList : AstRoot {
    AstExprList(
        llvm::SMRange range_,
        std::vector<AstExpr*>&& exprs_)
    : AstRoot{ AstKind::ExprList, range_ },
      exprs{ std::move(exprs_) } {}

    std::vector<AstExpr*> exprs;
};

struct AstAssignExpr final : AstExpr {
    AstAssignExpr(
        llvm::SMRange range_,
        AstExpr* lhs_,
        AstExpr* rhs_)
    : AstExpr{ AstKind::AssignExpr, range_ },
      lhs{ lhs_ },
      rhs{ rhs_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::AssignExpr;
    }

    AstExpr* lhs;
    AstExpr* rhs;
};

struct AstIdentExpr final : AstExpr {
    AstIdentExpr(
        llvm::SMRange range_,
        llvm::StringRef name_)
    : AstExpr{ AstKind::IdentExpr, range_ },
      name{ name_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::IdentExpr;
    }

    llvm::StringRef name;
    Symbol* symbol = nullptr;
};

struct AstCallExpr final : AstExpr {
    AstCallExpr(
        llvm::SMRange range_,
        AstExpr* callable_,
        AstExprList* args_)
    : AstExpr{ AstKind::CallExpr, range_ },
      callable{ callable_ },
      args{ args_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::CallExpr;
    }

    AstExpr* callable;
    AstExprList* args;
};

struct AstLiteralExpr final : AstExpr {
    using Value = Token::Value;

    AstLiteralExpr(
        llvm::SMRange range_,
        Value value_)
    : AstExpr{ AstKind::LiteralExpr, range_ },
      value{ value_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::LiteralExpr;
    }

    const Value value;
};

struct AstUnaryExpr final : AstExpr {
    AstUnaryExpr(
        llvm::SMRange range_,
        TokenKind tokenKind_,
        AstExpr* expr_)
    : AstExpr{ AstKind::UnaryExpr, range_ },
      tokenKind{ tokenKind_ },
      expr{ expr_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::UnaryExpr;
    }

    const TokenKind tokenKind;
    AstExpr* expr;
};

struct AstDereference final : AstExpr {
    AstDereference(
        llvm::SMRange range_,
        AstExpr* expr_)
    : AstExpr{ AstKind::Dereference, range_ },
      expr{ expr_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::Dereference;
    }

    AstExpr* expr;
};

struct AstAddressOf final : AstExpr {
    AstAddressOf(
        llvm::SMRange range_,
        AstExpr* expr_)
    : AstExpr{ AstKind::AddressOf, range_ },
      expr{ expr_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::AddressOf;
    }

    AstExpr* expr;
};

struct AstMemberAccess final : AstExpr {
    AstMemberAccess(
        llvm::SMRange range_,
        std::vector<AstExpr*>&& exprs_)
    : AstExpr{ AstKind::MemberAccess, range_ },
      exprs{ std::move(exprs_) } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::MemberAccess;
    }

    std::vector<AstExpr*> exprs;
};

struct AstBinaryExpr final : AstExpr {
    AstBinaryExpr(
        llvm::SMRange range_,
        TokenKind tokenKind_,
        AstExpr* lhs_,
        AstExpr* rhs_)
    : AstExpr{ AstKind::BinaryExpr, range_ },
      tokenKind{ tokenKind_ },
      lhs{ lhs_ },
      rhs{ rhs_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::BinaryExpr;
    }

    const TokenKind tokenKind;
    AstExpr* lhs;
    AstExpr* rhs;
};

struct AstCastExpr final : AstExpr {
    AstCastExpr(
        llvm::SMRange range_,
        AstExpr* expr_,
        AstTypeExpr* typeExpr_,
        bool implicit_)
    : AstExpr{ AstKind::CastExpr, range_ },
      expr{ expr_ },
      typeExpr{ typeExpr_ },
      implicit{ implicit_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::CastExpr;
    }

    AstExpr* expr;
    AstTypeExpr* typeExpr;
    const bool implicit;
};

struct AstIfExpr final : AstExpr {
    AstIfExpr(
        llvm::SMRange range_,
        AstExpr* expr_,
        AstExpr* trueExpr_,
        AstExpr* falseExpr_)
    : AstExpr{ AstKind::IfExpr, range_ },
      expr{ expr_ },
      trueExpr{ trueExpr_ },
      falseExpr{ falseExpr_ } {}

    constexpr static bool classof(const AstRoot* ast) {
        return ast->kind == AstKind::IfExpr;
    }

    AstExpr* expr;
    AstExpr* trueExpr;
    AstExpr* falseExpr;
};

#undef IS_AST_CLASSOF

} // namespace lbc
