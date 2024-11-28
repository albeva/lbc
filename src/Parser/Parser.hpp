//
// Created by Albert Varaksin on 03/07/2020.
//
#pragma once
#include "pch.hpp"
#include "Ast/Ast.def.hpp"
#include "Ast/ControlFlowStack.hpp"
#include "Diag/DiagnosticEngine.hpp"
#include "Lexer/Token.hpp"

namespace lbc {
class Context;
class SymbolTable;
class Lexer;
struct AstIfStmtBlock;
enum class CallingConv : std::uint8_t;
enum class AstContinuationAction : std::uint8_t;
AST_FORWARD_DECLARE()
struct AstExtern;

class Parser final : private ErrorLogger {
public:
    NO_COPY_AND_MOVE(Parser)

    enum class ExprFlags : std::uint8_t {
        commaAsAnd = 1U << 0U,
        useAssign = 1U << 1U,
        callWithoutParens = 1U << 2U
    };

    Parser(Context& context, Lexer& lexer, bool isMain, SymbolTable* symbolTable = nullptr);
    ~Parser() override = default;

    [[nodiscard]] auto parse() -> Result<AstModule*>;
    [[nodiscard]] auto expression(ExprFlags flags = {}) -> Result<AstExpr*>;
    [[nodiscard]] auto typeExpr() -> Result<AstTypeExpr*>;

    void reset();

private:
    enum class Scope : std::uint8_t {
        Root,
        Function
    };

    enum class FuncFlags : std::uint8_t {
        // Nameless declaration. For example in type definitions
        isAnonymous = 1U << 0U,
        // Is following DECLARE keyword
        isDeclaration = 1U << 1U,
    };

    [[nodiscard]] auto stmtList() -> Result<AstStmtList*>;
    [[nodiscard]] auto statement() -> Result<AstStmt*>;
    [[nodiscard]] auto kwImport() -> Result<AstImport*>;
    [[nodiscard]] auto kwExtern() -> Result<AstExtern*>;
    [[nodiscard]] auto declaration() -> Result<AstStmt*>;
    [[nodiscard]] auto factor() -> Result<AstExpr*>;
    [[nodiscard]] auto primary() -> Result<AstExpr*>;
    [[nodiscard]] auto unary(llvm::SMRange range, const Token& tkn, AstExpr* expr) const -> Result<AstExpr*>;
    [[nodiscard]] auto binary(llvm::SMRange range, const Token& tkn, AstExpr* lhs, AstExpr* rhs) const -> Result<AstExpr*>;
    [[nodiscard]] auto expression(AstExpr* lhs, int precedence) -> Result<AstExpr*>;
    [[nodiscard]] auto identifier() -> Result<AstIdentExpr*>;
    [[nodiscard]] auto literal() -> Result<AstLiteralExpr*>;
    [[nodiscard]] auto ifExpr() -> Result<AstIfExpr*>;
    [[nodiscard]] auto expressionList() -> Result<AstExprList*>;
    [[nodiscard]] auto kwDim(AstAttributeList* attribs) -> Result<AstVarDecl*>;
    [[nodiscard]] auto kwIf() -> Result<AstIfStmt*>;
    [[nodiscard]] auto ifBlock() -> Result<AstIfStmtBlock*>;
    [[nodiscard]] auto thenBlock(std::vector<AstVarDecl*> decls, AstExpr* expr) -> Result<AstIfStmtBlock*>;
    [[nodiscard]] auto kwFor() -> Result<AstForStmt*>;
    [[nodiscard]] auto kwDo() -> Result<AstDoLoopStmt*>;
    [[nodiscard]] auto kwContinue() -> Result<AstContinuationStmt*>;
    [[nodiscard]] auto kwExit() -> Result<AstContinuationStmt*>;
    [[nodiscard]] auto continuation(AstContinuationAction action) -> Result<AstContinuationStmt*>;
    [[nodiscard]] auto attributeList() -> Result<AstAttributeList*>;
    [[nodiscard]] auto attribute() -> Result<AstAttribute*>;
    [[nodiscard]] auto attributeArgList() -> Result<AstExprList*>;
    [[nodiscard]] auto kwTypeOf() -> Result<AstTypeOf*>;
    [[nodiscard]] auto kwDeclare(AstAttributeList* attribs) -> Result<AstFuncDecl*>;
    [[nodiscard]] auto funcSignature(llvm::SMLoc start, AstAttributeList* attribs, FuncFlags funcFlags) -> Result<AstFuncDecl*>;
    [[nodiscard]] auto funcParamList(bool& isVariadic, bool isAnonymous) -> Result<AstFuncParamList*>;
    [[nodiscard]] auto funcParam(bool isAnonymous) -> Result<AstFuncParamDecl*>;
    [[nodiscard]] auto kwFunction(AstAttributeList* attribs) -> Result<AstFuncStmt*>;
    [[nodiscard]] auto kwReturn() -> Result<AstStmt*>;
    [[nodiscard]] auto kwType(AstAttributeList* attribs) -> Result<AstDecl*>;
    [[nodiscard]] auto alias(llvm::StringRef id, Token token, llvm::SMLoc start, AstAttributeList* attribs) -> Result<AstTypeAlias*>;
    [[nodiscard]] auto udt(llvm::StringRef id, Token token, llvm::SMLoc start, AstAttributeList* attribs) -> Result<AstUdtDecl*>;
    [[nodiscard]] auto udtDeclList() -> Result<AstDeclList*>;
    [[nodiscard]] auto udtMember(AstAttributeList* attribs) -> Result<AstDecl*>;

    // replace token kind with another (e.g. Minus to Negate)
    void resolveBinaryOperators();

    // If token matches then advance and return true
    auto accept(TokenKind kind) -> bool;

    // Perform lookahead in the lexer, and if next token matches then advance and return true
    inline auto acceptNext(TokenKind kind) -> bool;

    // Expects a match, raises error when fails
    [[nodiscard]] auto expect(TokenKind kind) const -> Result<void>;

    // expects given token and advances.
    [[nodiscard]] auto consume(TokenKind kind) -> Result<void>;

    // advance to the next token from the stream
    void advance();

    Token m_token;
    Context& m_context;
    Lexer& m_lexer;
    const bool m_isMain;
    SymbolTable* m_symbolTable;
    CallingConv m_language;

    Scope m_scope;
    llvm::SMLoc m_endLoc;
    std::vector<AstImport*> m_imports;
    ExprFlags m_exprFlags {};
    ControlFlowStack<> m_controlStack {};
};
MARK_AS_FLAGS_ENUM(Parser::ExprFlags);
MARK_AS_FLAGS_ENUM(Parser::FuncFlags);

} // namespace lbc
