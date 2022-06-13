//
// Created by Albert Varaksin on 03/07/2020.
//
#pragma once
#include "Ast/Ast.def.hpp"
#include "Diag/DiagnosticEngine.hpp"
#include "Lexer/Token.hpp"

namespace lbc {
class Context;
class SymbolTable;
class TokenSource;
struct AstIfStmtBlock;
AST_FORWARD_DECLARE()

class Parser final {
public:
    NO_COPY_AND_MOVE(Parser)
    struct ExprFlags {
        bool commaAsAnd : 1;
        bool useAssign : 1;
        bool callWithoutParens : 1;
    };

    struct TypeFlags {
        bool typeOfAllowsExpr : 1;
    };

    Parser(Context& context, TokenSource& source, bool isMain, SymbolTable* symbolTable = nullptr);
    ~Parser() noexcept = default;

    [[nodiscard]] Result<AstModule*> parse();
    [[nodiscard]] Result<AstExpr*> expression(ExprFlags flags = {});
    [[nodiscard]] Result<AstTypeExpr*> typeExpr(TypeFlags flags = {});

    void reset() noexcept;
    [[nodiscard]] inline const Token& getToken() const noexcept { return m_token; }

private:
    enum class Scope {
        Root,
        Function
    };

    struct FuncFlags {
        // Nameless declaration. For example in type definitions
        bool isAnonymous : 1;
        // Is following DECLARE keyword
        bool isDeclaration : 1;
    };

    [[nodiscard]] Result<AstStmtList*> stmtList();
    [[nodiscard]] Result<AstStmt*> statement();
    [[nodiscard]] Result<AstImport*> kwImport();
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
    [[nodiscard]] Result<AstTypeAlias*> alias(llvm::StringRef id, llvm::SMLoc start, AstAttributeList* attribs);
    [[nodiscard]] Result<AstUdtDecl*> udt(llvm::StringRef id, llvm::SMLoc start, AstAttributeList* attribs);
    [[nodiscard]] Result<AstDeclList*> udtDeclList();
    [[nodiscard]] Result<AstDecl*> udtMember(AstAttributeList* attribs);

    // replace token kind with another (e.g. Minus to Negate)
    void replace(TokenKind what, TokenKind with) noexcept;
    void fixExprOperators() noexcept;

    // If token matches then advance and return true
    [[nodiscard]] bool accept(TokenKind kind) {
        if (m_token.is(kind)) {
            advance();
            return true;
        }
        return false;
    }

    // expects given token and advances.
    [[nodiscard]] Result<void> consume(TokenKind kind) noexcept {
        TRY(expect(kind))
        advance();
        return {};
    }

    // Expects a match, raises error when fails
    [[nodiscard]] Result<void> expect(TokenKind kind) const noexcept;

    // advance to the next token from the stream
    void advance();

    template<typename... Args>
    [[nodiscard]] Result<void> makeError(const llvm::SMRange& range, Diag diag, Args&&... args) const noexcept {
        m_diag.print(
            diag,
            range.Start,
            m_diag.format(diag, std::forward<Args>(args)...),
            range);
        return ResultError{};
    }

    template<typename... Args>
    [[nodiscard]] Result<void> makeError(Diag diag, Args&&... args) const noexcept {
        return makeError(m_token.range(), diag, std::forward<Args>(args)...);
    }

    Context& m_context;
    TokenSource& m_source;
    const bool m_isMain;
    SymbolTable* m_symbolTable;

    DiagnosticEngine& m_diag;
    Scope m_scope;
    Token m_token{};
    llvm::SMLoc m_endLoc{};
    std::vector<AstImport*> m_imports{};
    ExprFlags m_exprFlags{};
    TypeFlags m_typeFlags{};
};

} // namespace lbc
