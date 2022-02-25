//
// Created by Albert Varaksin on 03/07/2020.
//
#pragma once
#include "Ast/Ast.def.hpp"
#include "Diag/DiagnosticEngine.hpp"
#include "Lexer/Token.hpp"
#include "ParseError.hpp"

namespace lbc {
class Context;
class Lexer;
struct AstIfStmtBlock;
AST_FORWARD_DECLARE()

class Parser final {
public:
    NO_COPY_AND_MOVE(Parser)

    Parser(Context& context, unsigned int fileId, bool isMain);
    ~Parser() noexcept;

    [[nodiscard]] llvm::Expected<AstModule*> parse();

private:
    enum class Scope {
        Root,
        Function
    };

    enum class ExprFlags : unsigned {
        None = 0,
        CommaAsAnd = 1,
        UseAssign = 2,
        CallWithoutParens = 4,
        LLVM_MARK_AS_BITMASK_ENUM(/* LargestValue = */ CallWithoutParens)
    };

    [[nodiscard]] llvm::Expected<AstStmtList*> stmtList();
    [[nodiscard]] llvm::Expected<AstStmt*> statement();
    [[nodiscard]] llvm::Expected<AstImport*> kwImport();
    [[nodiscard]] llvm::Expected<AstStmt*> declaration();
    [[nodiscard]] llvm::Expected<AstExpr*> expression(ExprFlags flags = ExprFlags::None);
    [[nodiscard]] llvm::Expected<AstExpr*> factor();
    [[nodiscard]] llvm::Expected<AstExpr*> primary();
    [[nodiscard]] llvm::Expected<AstExpr*> unary(llvm::SMRange range, TokenKind op, AstExpr* expr);
    [[nodiscard]] llvm::Expected<AstExpr*> binary(llvm::SMRange range, TokenKind op, AstExpr* lhs, AstExpr* rhs);
    [[nodiscard]] llvm::Expected<AstExpr*> expression(AstExpr* lhs, int precedence);
    [[nodiscard]] llvm::Expected<AstIdentExpr*> identifier();
    [[nodiscard]] llvm::Expected<AstLiteralExpr*> literal();
    [[nodiscard]] llvm::Expected<AstCallExpr*> callExpr();
    [[nodiscard]] llvm::Expected<AstIfExpr*> ifExpr();
    [[nodiscard]] llvm::Expected<AstExprList*> expressionList();
    [[nodiscard]] llvm::Expected<AstVarDecl*> kwVar(AstAttributeList* attribs);
    [[nodiscard]] llvm::Expected<AstIfStmt*> kwIf();
    [[nodiscard]] llvm::Expected<AstIfStmtBlock*> ifBlock();
    [[nodiscard]] llvm::Expected<AstIfStmtBlock*> thenBlock(std::vector<AstVarDecl*> decls, AstExpr* expr);
    [[nodiscard]] llvm::Expected<AstForStmt*> kwFor();
    [[nodiscard]] llvm::Expected<AstDoLoopStmt*> kwDo();
    [[nodiscard]] llvm::Expected<AstContinuationStmt*> kwContinue();
    [[nodiscard]] llvm::Expected<AstContinuationStmt*> kwExit();
    [[nodiscard]] llvm::Expected<AstAttributeList*> attributeList();
    [[nodiscard]] llvm::Expected<AstAttribute*> attribute();
    [[nodiscard]] llvm::Expected<AstExprList*> attributeArgList();
    [[nodiscard]] llvm::Expected<AstTypeExpr*> typeExpr();
    [[nodiscard]] llvm::Expected<AstFuncDecl*> kwDeclare(AstAttributeList* attribs);
    [[nodiscard]] llvm::Expected<AstFuncDecl*> funcSignature(
        llvm::SMLoc start,
        AstAttributeList* attribs,
        bool hasImpl,
        bool isAnonymous = false);
    [[nodiscard]] llvm::Expected<AstFuncParamList*> funcParamList(bool& isVariadic, bool isAnonymous);
    [[nodiscard]] llvm::Expected<AstFuncParamDecl*> funcParam(bool isAnonymous);
    [[nodiscard]] llvm::Expected<AstFuncStmt*> kwFunction(AstAttributeList* attribs);
    [[nodiscard]] llvm::Expected<AstStmt*> kwReturn();
    [[nodiscard]] llvm::Expected<AstDecl*> kwType(AstAttributeList* attribs);
    [[nodiscard]] llvm::Expected<AstUdtDecl*> udt(AstAttributeList* attribs);
    [[nodiscard]] llvm::Expected<AstDeclList*> udtDeclList();
    [[nodiscard]] llvm::Expected<AstDecl*> udtMember(AstAttributeList* attribs);

    // replace token kind with another (e.g. Minus to Negate)
    void replace(TokenKind what, TokenKind with) noexcept;

    // If token matches then advance and return true
    [[nodiscard]] bool accept(TokenKind kind) {
        if (m_token.is(kind)) {
            advance();
            return true;
        }
        return false;
    }

    // expects given token and advances.
    [[nodiscard]] llvm::Error consume(TokenKind kind) noexcept {
        TRY(expect(kind))
        advance();
        return llvm::Error::success();
    }

    // Expects a match, raises error when fails
    [[nodiscard]] llvm::Error expect(TokenKind kind) noexcept;

    // advance to the next token from the stream
    void advance();

    template<typename... Args>
    [[nodiscard]] llvm::Error makeError(const llvm::SMRange& range, Diag diag, Args&&... args) const noexcept {
        return llvm::make_error<ParseError>(
            diag, m_diag.format(diag, std::forward<Args>(args)...), range);
    }

    template<typename... Args>
    [[nodiscard]] llvm::Error makeError(Diag diag, Args&&... args) noexcept {
        return makeError(m_token.range(), diag, std::forward<Args>(args)...);
    }

    Context& m_context;
    DiagnosticEngine& m_diag;
    const unsigned m_fileId;
    const bool m_isMain;
    Scope m_scope;
    unique_ptr<Lexer> m_lexer;
    Token m_token{};
    llvm::SMLoc m_endLoc{};
    ExprFlags m_exprFlags{};
};

} // namespace lbc
