//
// Created by Albert Varaksin on 03/07/2020.
//
#pragma once
#include "pch.hpp"
#include "Token.def.hpp"

namespace lbc {

enum class TokenKind {
#define IMPL_TOKENS(id, ...) id,
    ALL_TOKENS(IMPL_TOKENS)
#undef IMPL_TOKENS
};

enum class OperatorType {
    Arithmetic,
    Logical,
    Comparison,
    Memory,
    Assignment
};

class Token final {
public:
    using Value = std::variant<std::monostate, llvm::StringRef, uint64_t, double, bool>;

    // Describe given token kind
    static llvm::StringRef description(TokenKind kind);

    // find matching token for string or return TokenKind::Identifier
    static TokenKind findKind(llvm::StringRef str);

    // set token values
    void set(TokenKind kind, const llvm::SMRange& range, Value value = std::monostate{}) {
        m_kind = kind;
        m_range = range;
        m_value = value;
    }

    // Getters
    [[nodiscard]] TokenKind getKind() const { return m_kind; }
    void setKind(TokenKind kind) { m_kind = kind; }

    [[nodiscard]] llvm::StringRef lexeme() const;
    [[nodiscard]] std::string asString() const;
    [[nodiscard]] const Value& getValue() const { return m_value; }
    [[nodiscard]] llvm::StringRef getStringValue() const { return std::get<llvm::StringRef>(m_value); }
    [[nodiscard]] inline llvm::SMRange getRange() const { return m_range; }
    [[nodiscard]] llvm::StringRef description() const { return description(m_kind); }

    // Info about operators
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

    static OperatorType getOperatorType(TokenKind kind);

    // Query keyword types

    [[nodiscard]] bool isTypeKeyword() const;

    // comparisons

    [[nodiscard]] inline bool is(TokenKind kind) const {
        return m_kind == kind;
    }

    [[nodiscard]] inline bool isNot(TokenKind kind) const {
        return m_kind != kind;
    }

    [[nodiscard]] inline bool isOneOf(TokenKind k1, TokenKind k2) const {
        return is(k1) || is(k2);
    }

    template<typename... Ts>
    [[nodiscard]] inline bool isOneOf(TokenKind k1, TokenKind k2, Ts... ks) const {
        return is(k1) || isOneOf(k2, ks...);
    }

private:
    TokenKind m_kind = TokenKind::Invalid;
    Value m_value = std::monostate{};
    llvm::SMRange m_range{};
};

} // namespace lbc
