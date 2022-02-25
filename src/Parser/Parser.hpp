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
class Lexer;
struct AstIfStmtBlock;
AST_FORWARD_DECLARE()

class Parser final {
public:
    NO_COPY_AND_MOVE(Parser)

    Parser(Context& context, unsigned int fileId, bool isMain);
    ~Parser() noexcept;

    [[nodiscard]] ParseResult<AstModule> parse();

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

    [[nodiscard]] ParseResult<AstStmtList> stmtList();
    [[nodiscard]] ParseResult<AstStmt> statement();
    [[nodiscard]] ParseResult<AstImport> kwImport();
    [[nodiscard]] ParseResult<AstStmt> declaration();
    [[nodiscard]] ParseResult<AstExpr> expression(ExprFlags flags = ExprFlags::None);
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
    [[nodiscard]] ParseResult<AstVarDecl> kwVar(AstAttributeList* attribs);
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
    [[nodiscard]] ParseResult<AstTypeExpr> typeExpr();
    [[nodiscard]] ParseResult<AstFuncDecl> kwDeclare(AstAttributeList* attribs);
    [[nodiscard]] ParseResult<AstFuncDecl> funcSignature(
        llvm::SMLoc start,
        AstAttributeList* attribs,
        bool hasImpl,
        bool isAnonymous = false);
    [[nodiscard]] ParseResult<AstFuncParamList> funcParamList(bool& isVariadic, bool isAnonymous);
    [[nodiscard]] ParseResult<AstFuncParamDecl> funcParam(bool isAnonymous);
    [[nodiscard]] ParseResult<AstFuncStmt> kwFunction(AstAttributeList* attribs);
    [[nodiscard]] ParseResult<AstStmt> kwReturn();
    [[nodiscard]] ParseResult<AstDecl> kwType(AstAttributeList* attribs);
    [[nodiscard]] ParseResult<AstTypeAlias> alias(StringRef id, llvm::SMLoc start, AstAttributeList* attribs);
    [[nodiscard]] ParseResult<AstUdtDecl> udt(llvm::StringRef id, llvm::SMLoc start, AstAttributeList* attribs);
    [[nodiscard]] ParseResult<AstDeclList> udtDeclList();
    [[nodiscard]] ParseResult<AstDecl> udtMember(AstAttributeList* attribs);

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
    [[nodiscard]] ParseResult<void> consume(TokenKind kind) noexcept {
        TRY(expect(kind))
        advance();
        return {};
    }

    // Expects a match, raises error when fails
    [[nodiscard]] ParseResult<void> expect(TokenKind kind) noexcept;

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
    [[nodiscard]] ParseResult<void> makeError(Diag diag, Args&&... args) noexcept {
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
