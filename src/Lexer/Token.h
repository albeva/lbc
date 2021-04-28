//
// Created by Albert Varaksin on 03/07/2020.
//
#pragma once
#include "pch.h"
#include "Token.def.h"

namespace lbc {

enum class TokenKind {
#define IMPL_TOKENS(id, ...) id,
    ALL_TOKENS(IMPL_TOKENS)
#undef IMPL_TOKENS
};

class Token final : private NonCopyable {
public:
    /**
     * Create either identifier or a keyword token from string literal
     */
    static unique_ptr<Token> create(const llvm::StringRef& lexeme, const llvm::SMLoc& loc);

    /**
     * Create token with given kind and lexeme
     */
    static unique_ptr<Token> create(TokenKind kind, const llvm::StringRef& lexeme, const llvm::SMLoc& loc);

    /**
     * Create token with given kind and use description for lexeme
     */
    static unique_ptr<Token> create(TokenKind kind, const llvm::SMLoc& loc);

    /**
     * Get TokenKind string representation
     */
    static const llvm::StringRef& description(TokenKind kind);

    Token(TokenKind kind, const llvm::StringRef& lexeme, const llvm::SMLoc& loc)
    : m_kind{ kind }, m_lexeme{ lexeme }, m_loc{ loc } {}

    [[nodiscard]] inline TokenKind kind() const { return m_kind; }
    [[nodiscard]] inline const llvm::StringRef& lexeme() const { return m_lexeme; }

    [[nodiscard]] inline const llvm::SMLoc& loc() const { return m_loc; }
    [[nodiscard]] llvm::SMRange range() const;

    [[nodiscard]] const llvm::StringRef& description() const;

    [[nodiscard]] bool isGeneral() const;
    [[nodiscard]] bool isLiteral() const;
    [[nodiscard]] bool isSymbol() const;
    [[nodiscard]] bool isOperator() const;
    [[nodiscard]] bool isKeyword() const;

    [[nodiscard]] int getPrecedence() const;
    [[nodiscard]] bool isBinary() const;
    [[nodiscard]] bool isUnary() const;
    [[nodiscard]] bool isLeftToRight() const;
    [[nodiscard]] bool isRightToLeft() const;

    inline bool operator==(TokenKind rhs) const {
        return m_kind == rhs;
    }

    inline bool operator!=(TokenKind rhs) const {
        return m_kind != rhs;
    }

private:
    const TokenKind m_kind;
    const llvm::StringRef m_lexeme;
    const llvm::SMLoc m_loc;
};

} // namespace lbc
