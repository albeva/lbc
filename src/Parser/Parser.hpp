//
// Created by Albert Varaksin on 12/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Ast/AstFwdDecl.hpp"
#include "Diag/DiagEngine.hpp"
#include "Diag/LogProvider.hpp"
#include "Lexer/Lexer.hpp"
#include "Lexer/Token.hpp"
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
    [[nodiscard]] auto getContext() -> Context& { return m_context; }

private:
    // --------------------------------
    // Utilities
    // --------------------------------

    /**
     * Abort compilation with a fatal diagnostic.
     */
    [[noreturn]] static void panic(DiagMessage message);

    /**
     * Create an error indicating the current token was unexpected.
     */
    [[nodiscard]] auto unexpected(std::source_location location = std::source_location::current()) -> DiagError;

    /**
     * Create an error indicating unimplemented functionality.
     */
    [[nodiscard]] auto notImplemented(std::source_location location = std::source_location::current()) -> DiagError;

    /**
     * Consume the current token and advance to the next one.
     */
    void advance();

    /**
     * If the current token matches the given kind, consume it
     * and return true. Otherwise leave it and return false.
     */
    [[nodiscard]] auto accept(TokenKind kind) -> bool;

    /**
     * Assert that the current token is of the given kind.
     * Returns an error if it does not match.
     */
    [[nodiscard]] auto expect(TokenKind kind) -> Result<void>;

    /**
     * Expect the current token to match the given kind, then advance.
     */
    [[nodiscard]] auto consume(TokenKind kind) -> Result<void>;

    // --------------------------------
    // Declarations (ParseDecl.cpp)
    // --------------------------------

    [[nodiscard]] auto declaration() -> Result<void>;

    // --------------------------------
    // Statements (ParseStmt.cpp)
    // --------------------------------

    [[nodiscard]] auto statement() -> Result<void>;

    // --------------------------------
    // Expressions (ParseExpr.cpp)
    // --------------------------------

    [[nodiscard]] auto expression() -> Result<void>;

    // --------------------------------
    // Types (ParseType.cpp)
    // --------------------------------

    [[nodiscard]] auto type() -> Result<void>;

    // --------------------------------
    // Data
    // --------------------------------
    Context& m_context;
    Lexer m_lexer;
    Token m_token;
};

} // namespace lbc
