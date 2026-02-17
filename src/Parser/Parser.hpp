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

    enum class Scope : std::uint8_t {
        Module,       // top-level module scope
        ImplicitMain, // once non-declaration is encountered, scope becomes implicit main
        Sub,          // subroutine scope
        Function,     // function scope
    };

    // -------------------------------------------------------------------------
    // Error Handling
    // -------------------------------------------------------------------------

    /**
     * Create an error indicating the current token was unexpected.
     */
    [[nodiscard]] auto unexpected(const std::source_location& location = std::source_location::current()) -> DiagError;

    /**
     * Create an error reporting what was expected vs. what was found.
     */
    [[nodiscard]] auto expected(const diagnostics::Loggable auto& expected, const std::source_location& location = std::source_location::current()) -> DiagError {
        return diag(diagnostics::expected(expected, m_token), m_token.getRange().Start, m_token.getRange(), location);
    }

    /**
     * Create an error indicating unimplemented functionality.
     */
    [[nodiscard]] auto notImplemented(const std::source_location& location = std::source_location::current()) -> DiagError;

    // -------------------------------------------------------------------------
    // Parsing basics
    // -------------------------------------------------------------------------

    /**
     * Consume the current token and advance to the next one.
     */
    [[nodiscard]] auto advance() -> Result<void>;

    /**
     * If the current token matches the given kind, consume it
     * and return true. Otherwise leave it and return false.
     */
    [[nodiscard]] auto accept(TokenKind kind) -> Result<bool>;

    /**
     * Assert that the current token is of the given kind.
     * Returns an error if it does not match.
     */
    [[nodiscard]] auto expect(TokenKind kind) -> Result<void>;

    /**
     * Expect the current token to match the given kind, then advance.
     */
    [[nodiscard]] auto consume(TokenKind kind) -> Result<void>;

    // -------------------------------------------------------------------------
    // Source location and range
    // -------------------------------------------------------------------------

    [[nodiscard]] auto start() const -> llvm::SMLoc { return m_token.getRange().Start; }
    [[nodiscard]] auto end() const -> llvm::SMLoc { return m_token.getRange().End; }

    [[nodiscard]] auto range() const -> llvm::SMRange {
        return { start(), end() };
    }

    [[nodiscard]] auto range(const llvm::SMLoc start) const -> llvm::SMRange {
        return { start, end() };
    }

    [[nodiscard]] auto range(const AstRoot* start) const -> llvm::SMRange {
        return { start->getRange().Start, end() };
    }

    [[nodiscard]] static auto range(const AstRoot* first, const AstRoot* last) -> llvm::SMRange {
        return { first->getRange().Start, last->getRange().End };
    }

    // -------------------------------------------------------------------------
    // Memory handling
    // -------------------------------------------------------------------------

    template <typename T, typename... Args>
    auto make(Args&&... args) -> T* {
        return getContext().create<T>(std::forward<Args>(args)...);
    }

    template <typename T>
    [[nodiscard]] auto sequence(Sequencer<T>& seq) -> std::span<T*> {
        return seq.sequence(getContext());
    }

    // -------------------------------------------------------------------------
    // Declarations (ParseDecl.cpp)
    // -------------------------------------------------------------------------

    [[nodiscard]] auto declaration() -> Result<void>;

    // -------------------------------------------------------------------------
    // Statements (ParseStmt.cpp)
    // -------------------------------------------------------------------------

    [[nodiscard]] auto stmtList() -> Result<AstStmtList*>;
    [[nodiscard]] auto statement() -> Result<void>;

    // -------------------------------------------------------------------------
    // Expressions (ParseExpr.cpp)
    // -------------------------------------------------------------------------

    [[nodiscard]] auto expression() -> Result<void>;

    // -------------------------------------------------------------------------
    // Types (ParseType.cpp)
    // -------------------------------------------------------------------------

    [[nodiscard]] auto type() -> Result<void>;

    // -------------------------------------------------------------------------
    // Parser Data
    // -------------------------------------------------------------------------
    // the lexer
    Lexer m_lexer;
    // current token
    Token m_token;
    // parsing scope
    Scope m_scope = Scope::Module;
};

} // namespace lbc
