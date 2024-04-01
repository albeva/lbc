//
// Created by Albert Varaksin on 03/07/2020.
//
#pragma once
#include "pch.hpp"
#include "Ast/Ast.def.hpp"
#include "Diag/DiagnosticEngine.hpp"
#include "Lexer/Token.hpp"

namespace lbc {
class Context;
class SymbolTable;
class TokenSource;
struct AstIfStmtBlock;
enum class CallingConv;
AST_FORWARD_DECLARE()
struct AstExtern;

class Parser final {
public:
    NO_COPY_AND_MOVE(Parser)

    enum class ExprFlags : unsigned {
        commaAsAnd = 1U << 0U,
        useAssign = 1U << 1U,
        callWithoutParens = 1U << 2U
    };

    Parser(Context& context, TokenSource& source, bool isMain, SymbolTable* symbolTable = nullptr);
    ~Parser() = default;

    [[nodiscard]] Result<AstModule*> parse();
    [[nodiscard]] Result<AstExpr*> expression(ExprFlags flags = {});
    [[nodiscard]] Result<AstTypeExpr*> typeExpr();

    void reset();
    [[nodiscard]] inline const Token& getToken() const { return m_token; }

private:
    enum class Scope : unsigned {
        Root,
        Function
    };

    enum class FuncFlags : unsigned {
        // Nameless declaration. For example in type definitions
        isAnonymous = 1U << 0U,
        // Is following DECLARE keyword
        isDeclaration = 1U << 1U,
    };

    [[nodiscard]] Result<AstStmtList*> stmtList();
    [[nodiscard]] Result<AstStmt*> statement();
    [[nodiscard]] Result<AstImport*> kwImport();
    [[nodiscard]] Result<AstExtern*> kwExtern();
    [[nodiscard]] Result<AstStmt*> declaration();
    [[nodiscard]] Result<AstExpr*> factor();
    [[nodiscard]] Result<AstExpr*> primary();
    [[nodiscard]] Result<AstExpr*> unary(llvm::SMRange range, TokenKind op, AstExpr* expr);
    [[nodiscard]] Result<AstExpr*> binary(llvm::SMRange range, TokenKind op, AstExpr* lhs, AstExpr* rhs);
    [[nodiscard]] Result<AstExpr*> expression(AstExpr* lhs, int precedence);
    [[nodiscard]] Result<AstIdentExpr*> identifier();
    [[nodiscard]] Result<AstLiteralExpr*> literal();
    [[nodiscard]] Result<AstCallExpr*> callExpr();
    [[nodiscard]] Result<AstIfExpr*> ifExpr();
    [[nodiscard]] Result<AstExprList*> expressionList();
    [[nodiscard]] Result<AstVarDecl*> kwDim(AstAttributeList* attribs);
    [[nodiscard]] Result<AstIfStmt*> kwIf();
    [[nodiscard]] Result<AstIfStmtBlock*> ifBlock();
    [[nodiscard]] Result<AstIfStmtBlock*> thenBlock(std::vector<AstVarDecl*> decls, AstExpr* expr);
    [[nodiscard]] Result<AstForStmt*> kwFor();
    [[nodiscard]] Result<AstDoLoopStmt*> kwDo();
    [[nodiscard]] Result<AstContinuationStmt*> kwContinue();
    [[nodiscard]] Result<AstContinuationStmt*> kwExit();
    [[nodiscard]] Result<AstAttributeList*> attributeList();
    [[nodiscard]] Result<AstAttribute*> attribute();
    [[nodiscard]] Result<AstExprList*> attributeArgList();
    [[nodiscard]] Result<AstTypeOf*> kwTypeOf();
    [[nodiscard]] Result<AstFuncDecl*> kwDeclare(AstAttributeList* attribs);
    [[nodiscard]] Result<AstFuncDecl*> funcSignature(llvm::SMLoc start, AstAttributeList* attribs, FuncFlags funcFlags);
    [[nodiscard]] Result<AstFuncParamList*> funcParamList(bool& isVariadic, bool isAnonymous);
    [[nodiscard]] Result<AstFuncParamDecl*> funcParam(bool isAnonymous);
    [[nodiscard]] Result<AstFuncStmt*> kwFunction(AstAttributeList* attribs);
    [[nodiscard]] Result<AstStmt*> kwReturn();
    [[nodiscard]] Result<AstDecl*> kwType(AstAttributeList* attribs);
    [[nodiscard]] Result<AstTypeAlias*> alias(llvm::StringRef id, Token token, llvm::SMLoc start, AstAttributeList* attribs);
    [[nodiscard]] Result<AstUdtDecl*> udt(llvm::StringRef id, Token token, llvm::SMLoc start, AstAttributeList* attribs);
    [[nodiscard]] Result<AstDeclList*> udtDeclList();
    [[nodiscard]] Result<AstDecl*> udtMember(AstAttributeList* attribs);

    // replace token kind with another (e.g. Minus to Negate)
    void replace(TokenKind what, TokenKind with);
    void fixExprOperators();

    // If token matches then advance and return true
    [[nodiscard]] bool accept(TokenKind kind) {
        if (m_token.is(kind)) {
            advance();
            return true;
        }
        return false;
    }

    // expects given token and advances.
    [[nodiscard]] Result<void> consume(TokenKind kind) {
        TRY(expect(kind))
        advance();
        return {};
    }

    // Expects a match, raises error when fails
    [[nodiscard]] Result<void> expect(TokenKind kind) const;

    // advance to the next token from the stream
    void advance();

    template<typename... Args>
    [[nodiscard]] ResultError makeError(Diag diag, Args&&... args) const {
        return m_diag.makeError(diag, m_token.range(), std::forward<Args>(args)...);
    }

    Context& m_context;
    TokenSource& m_source;
    const bool m_isMain;
    SymbolTable* m_symbolTable;
    CallingConv m_language;

    DiagnosticEngine& m_diag;
    Scope m_scope;
    Token m_token{};
    llvm::SMLoc m_endLoc{};
    std::vector<AstImport*> m_imports{};
    ExprFlags m_exprFlags{};
};
MARK_AS_FLAGS_ENUM(Parser::ExprFlags);
MARK_AS_FLAGS_ENUM(Parser::FuncFlags);

} // namespace lbc
