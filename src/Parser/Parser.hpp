//
// Created by Albert Varaksin on 03/07/2020.
//
#pragma once
#include "Ast/Ast.def.hpp"
#include "Diag/DiagnosticEngine.hpp"
#include "Lexer/Token.hpp"
#include "ParseResult.hpp"

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

    [[nodiscard]] ParseResult<AstModule> parse();
    [[nodiscard]] ParseResult<AstExpr> expression(ExprFlags flags = {});
    [[nodiscard]] ParseResult<AstTypeExpr> typeExpr(TypeFlags flags = {});

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
        // Allow single line statement function() as integer => 42
        bool allowShorthand : 1;
        // Allow getting type from function body
        bool allowUntyped : 1;
    };

    [[nodiscard]] ParseResult<AstStmtList> stmtList();
    [[nodiscard]] ParseResult<AstStmt> statement();
    [[nodiscard]] ParseResult<AstImport> kwImport();
    [[nodiscard]] ParseResult<AstStmt> declaration();
    [[nodiscard]] ParseResult<AstExpr> factor();
    [[nodiscard]] ParseResult<AstExpr> primary();
    [[nodiscard]] ParseResult<AstExpr> unary(llvm::SMRange range, TokenKind op, AstExpr* expr);
    [[nodiscard]] ParseResult<AstExpr> binary(llvm::SMRange range, TokenKind op, AstExpr* lhs, AstExpr* rhs);
    [[nodiscard]] ParseResult<AstExpr> expression(AstExpr* lhs, int precedence);
    [[nodiscard]] ParseResult<AstIdentExpr> identifier();
    [[nodiscard]] ParseResult<AstLiteralExpr> literal();
    [[nodiscard]] ParseResult<AstCallExpr> callExpr();
    [[nodiscard]] ParseResult<AstIfExpr> ifExpr();
    [[nodiscard]] ParseResult<AstExprList> expressionList();
    [[nodiscard]] ParseResult<AstVarDecl> kwDim(AstAttributeList* attribs);
    [[nodiscard]] ParseResult<AstIfStmt> kwIf();
    [[nodiscard]] ParseResult<AstIfStmtBlock> ifBlock();
    [[nodiscard]] ParseResult<AstIfStmtBlock> thenBlock(std::vector<AstVarDecl*> decls, AstExpr* expr);
    [[nodiscard]] ParseResult<AstForStmt> kwFor();
    [[nodiscard]] ParseResult<AstDoLoopStmt> kwDo();
    [[nodiscard]] ParseResult<AstContinuationStmt> kwContinue();
    [[nodiscard]] ParseResult<AstContinuationStmt> kwExit();
    [[nodiscard]] ParseResult<AstAttributeList> attributeList();
    [[nodiscard]] ParseResult<AstAttribute> attribute();
    [[nodiscard]] ParseResult<AstExprList> attributeArgList();
    [[nodiscard]] ParseResult<AstTypeOf> kwTypeOf();
    [[nodiscard]] ParseResult<AstFuncDecl> kwDeclare(AstAttributeList* attribs);
    [[nodiscard]] ParseResult<AstFuncDecl> funcSignature(llvm::SMLoc start, AstAttributeList* attribs, FuncFlags funcFlags);
    [[nodiscard]] ParseResult<AstFuncParamList> funcParamList(bool& isVariadic, bool isAnonymous);
    [[nodiscard]] ParseResult<AstFuncParamDecl> funcParam(bool isAnonymous);
    [[nodiscard]] ParseResult<AstFuncStmt> kwFunction(AstAttributeList* attribs);
    [[nodiscard]] ParseResult<AstStmt> kwReturn();
    [[nodiscard]] ParseResult<AstDecl> kwType(AstAttributeList* attribs);
    [[nodiscard]] ParseResult<AstTypeAlias> alias(llvm::StringRef id, llvm::SMLoc start, AstAttributeList* attribs);
    [[nodiscard]] ParseResult<AstUdtDecl> udt(llvm::StringRef id, llvm::SMLoc start, AstAttributeList* attribs);
    [[nodiscard]] ParseResult<AstDeclList> udtDeclList();
    [[nodiscard]] ParseResult<AstDecl> udtMember(AstAttributeList* attribs);

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
    [[nodiscard]] ParseResult<void> consume(TokenKind kind) noexcept {
        TRY(expect(kind))
        advance();
        return {};
    }

    // Expects a match, raises error when fails
    [[nodiscard]] ParseResult<void> expect(TokenKind kind) const noexcept;

    // advance to the next token from the stream
    void advance();

    template<typename... Args>
    [[nodiscard]] ParseResult<void> makeError(const llvm::SMRange& range, Diag diag, Args&&... args) const noexcept {
        m_diag.print(
            diag,
            range.Start,
            m_diag.format(diag, std::forward<Args>(args)...),
            range);
        return ParseResult<void>::error();
    }

    template<typename... Args>
    [[nodiscard]] ParseResult<void> makeError(Diag diag, Args&&... args) const noexcept {
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
    ExprFlags m_exprFlags{};
    TypeFlags m_typeFlags{};
};

} // namespace lbc
