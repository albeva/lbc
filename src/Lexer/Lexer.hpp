//
// Created by Albert Varaksin on 12/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Cursor.hpp"
#include "Diag/LogProvider.hpp"
#include "Token.hpp"
namespace lbc {
class Context;

class Lexer final : LogProvider {
public:
    NO_COPY_AND_MOVE(Lexer)
    Lexer(Context& context, unsigned id);

    /**
     * Return the source buffer ID associated with this lexer.
     */
    [[nodiscard]] auto getId() const -> unsigned { return m_id; }

    /**
     * Get next token from the input.
     */
    [[nodiscard]] auto next() -> DiagResult<Token>;

    /**
     * Peek at the next token without consuming it.
     */
    [[nodiscard]] auto peek() -> DiagResult<Token>;

    /**
     * Get associated context object
     */
    [[nodiscard]] auto getContext() -> Context& { return m_context; }

private:
    /**
     * Create an invalid token at the current position.
     */
    [[nodiscard]] auto invalid() -> DiagError;

    /**
     * Create an end-of-file token.
     */
    [[nodiscard]] auto endOfFile() -> Token;

    /**
     * Create an end-of-statement token.
     */
    [[nodiscard]] auto endOfStmt() -> Token;

    /**
     * Create a token for an operator or punctuation of the given length.
     */
    [[nodiscard]] auto make(TokenKind kind, std::size_t len = 1) -> Token;

    /**
     * Create a token with an associated literal value.
     */
    [[nodiscard]] auto token(TokenKind kind, LiteralValue value = {}) -> Token;

    /**
     * Skip characters until a line ending or end of file.
     */
    void skipUntilLineEnd();

    /**
     * Skip a nested multiline comment (/' ... '/).
     */
    void skipMultilineComment();

    /**
     * Skip remaining characters on the current line and consume the newline.
     */
    void skipToNextLine();

    /**
     * Lex an identifier or keyword.
     */
    [[nodiscard]] auto identifier() -> DiagResult<Token>;

    /**
     * Lex a double-quoted string literal.
     */
    [[nodiscard]] auto stringLiteral() -> DiagResult<Token>;

    /**
     * Lex an integer or floating-point number literal.
     */
    [[nodiscard]] auto numberLiteral() -> DiagResult<Token>;

    /**
     * Return the source range from m_start to m_input.
     */
    [[nodiscard]] auto range() const -> llvm::SMRange {
        return m_start.rangeTo(m_input);
    }

    /**
     * Return the source text from m_start to m_input.
     */
    [[nodiscard]] auto lexeme() const -> llvm::StringRef {
        return m_start.stringTo(m_input);
    }

    /**
     * Parse the lexeme between m_start and m_input as a number.
     */
    template <typename T>
        requires std::is_arithmetic_v<T>
    [[nodiscard]] auto number() const -> std::optional<T> {
        T value;
        if (std::from_chars(m_start.data(), m_input.data(), value).ec == std::errc {}) { // NOLINT(*-invalid-enum-default-initialization)
            return { value };
        }
        return std::nullopt;
    }

    Context& m_context;
    unsigned m_id;
    Cursor m_start, m_input;
    bool m_hasStatement;
    std::string m_buffer;
};

} // namespace lbc
