//
// Created by Albert Varaksin on 12/02/2026.
//
#pragma once
#include "pch.hpp"

#include "Ast/Ast.hpp"
#include "Ast/AstFwdDecl.hpp"
#include "Diag/DiagEngine.hpp"
#include "Diag/LogProvider.hpp"
#include "Lexer/Lexer.hpp"
#include "Lexer/Token.hpp"
#include "Utilities/Sequencer.hpp"
namespace lbc {
class Context;

/**
 * Recursive-descent parser. Implementation is split across
 * multiple .cpp files by concern: ParseDecl, ParseExpr, ParseStmt,
 * ParseType, and Parser.cpp for common utilities.
 *
 * Grammar (EBNF):
 * @code
 * module     = stmtList EOF .
 * stmtList   = { statement EOS } .
 * statement  = declareStmt | dimStmt .
 * dimStmt    = "DIM" varDecl { "," varDecl } .
 * varDecl    = id ( "AS" typeExpr [ "=" expression ] | "=" expression ) .
 * expression = primary { <binary-op> primary } .
 * primary    = variable | literal | "(" expression ")" | prefix .
 * variable   = id .
 * literal    = "null" | "true" | "false" | <integer> | <float> | <string> .
 * prefix     = <unary-op> primary .
 * sub        = callee [ params ] .
 * function   = callee "(" [ params ] ")" .
 * params     = expression { "," expression } .
 * @endcode
 */
class Parser final : protected LogProvider {
public:
    NO_COPY_AND_MOVE(Parser);

    /**
     * Lightweight result type for fallible parser operations.
     */
    template <typename T>
    using Result = DiagResult<T>;

    /**
     * Construct a parser for the source buffer identified by @param id.
     */
    Parser(Context& context, unsigned id);
    ~Parser();

    /**
     * Run the parser over the source buffer.
     */
    auto parse() -> Result<AstModule*>;

    /**
     * Get associated context object
     */
    [[nodiscard]] auto getContext() -> Context& { return m_lexer.getContext(); }

private:
    // --------------------------------
    // Utilities
    // --------------------------------

    /// Current parsing scope, determines which constructs are valid.
    enum class Scope : std::uint8_t {
        Module,       ///< top-level module scope
        ImplicitMain, ///< once a non-declaration is encountered, scope becomes implicit main
        Sub,          ///< subroutine scope
        Function,     ///< function scope
    };

    /// Flags that modify expression parsing behaviour.
    struct ExprFlags {
        bool callWithoutParens : 1 = false; ///< allow paren-free subroutine call syntax
        bool stopAtAssign      : 1 = false; ///< stop before `=` so assignment is handled at statement level
    };

    /// Default expression flags used when none are explicitly provided.
    constexpr static ExprFlags defaultExprFlags { .callWithoutParens = false, .stopAtAssign = false };

    // -------------------------------------------------------------------------
    // Error Handling
    // -------------------------------------------------------------------------

    /**
     * Create an error indicating the current token was unexpected.
     * If a deferred lexer error is pending, returns that instead.
     */
    [[nodiscard]] auto unexpected(const std::source_location& location = std::source_location::current()) -> DiagError;

    /**
     * Create an error reporting what was @param expected vs. what was found.
     */
    [[nodiscard]] auto expected(const auto& expected, const std::source_location& location = std::source_location::current()) -> DiagError {
        return diag(diagnostics::expected(expected, m_token), m_token.getRange().Start, m_token.getRange(), location);
    }

    // -------------------------------------------------------------------------
    // Parsing basics
    // -------------------------------------------------------------------------

    /**
     * Consume the current token and advance to the next one.
     *
     * If the lexer produces an error, the token is set to Invalid and
     * the error is stored in m_deferredError rather than returned
     * immediately. This allows valid subexpressions to complete before
     * the error surfaces. The deferred error is returned on the next
     * call to advance(), or when expect() / unexpected() encounters
     * the Invalid token. m_lastLoc is only updated on success, so it
     * always reflects the end of the last successfully consumed token.
     */
    [[nodiscard]] auto advance() -> Result<void>;

    /**
     * If the current token matches @param kind, consume it and
     * return true. Otherwise leave the token in place and return false.
     */
    [[nodiscard]] auto accept(TokenKind kind) -> Result<bool>;

    /**
     * Verify that the current token matches @param kind without
     * consuming it. If a deferred lexer error is pending, returns
     * that. Otherwise returns an "expected X" diagnostic.
     */
    [[nodiscard]] auto expect(TokenKind kind) -> Result<void>;

    /**
     * Expect the current token to match @param kind, then advance
     * past it. Equivalent to expect() followed by advance().
     */
    [[nodiscard]] auto consume(TokenKind kind) -> Result<void>;

    // -------------------------------------------------------------------------
    // Generic grammars
    // -------------------------------------------------------------------------

    /**
     * Expect the current token to be an identifier, consume it,
     * and return its string value.
     */
    [[nodiscard]] auto identifier() -> Result<llvm::StringRef>;

    // -------------------------------------------------------------------------
    // Source location and range
    // -------------------------------------------------------------------------

    /** Return the start location of the current token. */
    [[nodiscard]] auto startLoc() const -> llvm::SMLoc { return m_token.getRange().Start; }

    /** Build a range from @param start to the end of the last consumed token. */
    [[nodiscard]] auto range(const llvm::SMLoc start) const -> llvm::SMRange {
        return { start, m_lastLoc };
    }

    /** Build a range from the start of @param start node to the end of the last consumed token. */
    [[nodiscard]] auto range(const AstRoot* start) const -> llvm::SMRange {
        return { start->getRange().Start, m_lastLoc };
    }

    /** Build a range spanning from @param first to @param last node. */
    [[nodiscard]] static auto range(const AstRoot* first, const AstRoot* last) -> llvm::SMRange {
        return { first->getRange().Start, last->getRange().End };
    }

    // -------------------------------------------------------------------------
    // Memory handling
    // -------------------------------------------------------------------------

    /** Arena-allocate an AST node via the context. */
    template <typename T, typename... Args>
    auto make(Args&&... args) -> T* {
        return getContext().create<T>(std::forward<Args>(args)...);
    }

    /** Flatten a sequencer into a contiguous arena-allocated span. */
    template <typename T>
    [[nodiscard]] auto sequence(Sequencer<T>& seq) -> std::span<T*> {
        return seq.sequence(getContext());
    }

    // -------------------------------------------------------------------------
    // Declarations (ParseDecl.cpp)
    // -------------------------------------------------------------------------

    /** Parse a variable declaration (name, optional type, optional initializer). */
    [[nodiscard]] auto varDecl() -> Result<AstVarDecl*>;

    /** Parse a subroutine declaration (name and optional parameter list, no return type). */
    [[nodiscard]] auto subDecl() -> Result<AstFuncDecl*>;

    /** Parse a function declaration (name, parameter list, and return type). */
    [[nodiscard]] auto funcDecl() -> Result<AstFuncDecl*>;

    /** Parse a comma-separated parameter declaration list. */
    [[nodiscard]] auto paramList() -> Result<std::span<AstFuncParamDecl*>>;

    /** Parse a single parameter declaration (name and type). */
    [[nodiscard]] auto paramDecl() -> Result<AstFuncParamDecl*>;

    // -------------------------------------------------------------------------
    // Statements (ParseStmt.cpp)
    // -------------------------------------------------------------------------

    /** Parse a list of statements terminated by a block-ending token. */
    [[nodiscard]] auto stmtList() -> Result<AstStmtList*>;

    /** Parse a single statement. */
    [[nodiscard]] auto statement() -> Result<AstStmt*>;

    /** Parse a DIM statement with one or more variable declarations. */
    [[nodiscard]] auto dimStmt() -> Result<AstStmt*>;

    /** Parse a DECLARE forward-declaration statement. */
    [[nodiscard]] auto declareStmt() -> Result<AstStmt*>;

    // -------------------------------------------------------------------------
    // Expressions (ParseExpr.cpp)
    // -------------------------------------------------------------------------

    /** Parse an expression using precedence climbing. */
    [[nodiscard]] auto expression(ExprFlags flags = defaultExprFlags) -> Result<AstExpr*>;

    /** Parse a primary expression (variable, literal, parenthesised, or prefix). */
    [[nodiscard]] auto primaryExpr() -> Result<AstExpr*>;

    /** Parse a variable reference. */
    [[nodiscard]] auto varExpr() -> Result<AstExpr*>;

    /** Parse a literal value (integer, float, boolean, string, or null). */
    [[nodiscard]] auto literalExpr() -> Result<AstExpr*>;

    /** Parse a paren-free subroutine call: `callee arg1, arg2`. */
    [[nodiscard]] auto subCallExpr(AstExpr* callee) -> Result<AstExpr*>;

    /** Parse a parenthesised function call: `callee(arg1, arg2)`. */
    [[nodiscard]] auto funcCallExpr(AstExpr* callee) -> Result<AstExpr*>;

    /** Parse a comma-separated argument list. */
    [[nodiscard]] auto argExprList() -> Result<std::span<AstExpr*>>;

    /** Parse a prefix unary expression. */
    [[nodiscard]] auto prefixExpr() -> Result<AstExpr*>;

    /** Parse a suffix expression (function call, type cast). */
    [[nodiscard]] auto suffixExpr(AstExpr* lhs) -> Result<AstExpr*>;

    /** Construct the appropriate binary or member-access AST node. */
    [[nodiscard]] auto binaryExpr(AstExpr* lhs, AstExpr* rhs, TokenKind tkn) -> Result<AstExpr*>;

    /** Precedence-climbing loop. Parses binary and suffix operators at or above @param precedence. */
    [[nodiscard]] auto climb(AstExpr* lhs, int precedence = 1) -> Result<AstExpr*>;

    /** Check whether expression parsing should stop before the current token. */
    [[nodiscard]] auto shouldBreak() const -> bool;

    // -------------------------------------------------------------------------
    // Types (ParseType.cpp)
    // -------------------------------------------------------------------------

    /** Parse a type expression. */
    [[nodiscard]] auto type() -> Result<AstType*>;
    [[nodiscard]] auto builtinType() -> Result<AstBuiltInType*>;

    // -------------------------------------------------------------------------
    // Parser Data
    // -------------------------------------------------------------------------
    Lexer m_lexer;                            ///< Lexer producing tokens from the source buffer
    Token m_token;                            ///< Currently read token (one-token lookahead)
    llvm::SMLoc m_lastLoc;                    ///< End location of the last consumed token
    DiagIndex m_deferredError;                ///< Lexer error deferred until token is demanded
    Scope m_scope = Scope::Module;            ///< Current parsing scope
    ExprFlags m_exprFlags = defaultExprFlags; ///< Active expression parsing flags
};

} // namespace lbc
