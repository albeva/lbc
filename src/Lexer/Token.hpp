//
// Created by Albert Varaksin on 03/07/2020.
//
#pragma once
#include "pch.hpp"
#include "Token.def.hpp"
#include "Utils/TokenValue.hpp"

namespace lbc {

// clang-format off
enum class TokenKind : std::uint8_t {
    #define IMPL_TOKENS(id, ...) id,
    ALL_TOKENS(IMPL_TOKENS)
    #undef IMPL_TOKENS
};
// clang-format on

enum class OperatorType : std::uint8_t {
    Arithmetic,
    Logical,
    Comparison,
    Cast,
    Memory,
    Assignment
};

class Token final {
public:

    // Describe given token kind
    static auto description(TokenKind kind) -> llvm::StringRef;

    // find matching token for string or return TokenKind::Identifier
    static auto findKind(llvm::StringRef str) -> TokenKind;

    // Default
    constexpr Token() = default;

    // set token values
    constexpr Token(TokenKind kind, const llvm::SMRange& range, TokenValue value = TokenValue::NullType {})
    : m_kind(kind)
    , m_value(value)
    , m_range(range) { }

    // Getters
    [[nodiscard]] constexpr auto getKind() const -> TokenKind { return m_kind; }
    constexpr void setKind(TokenKind kind) { m_kind = kind; }

    [[nodiscard]] auto lexeme() const -> llvm::StringRef;
    [[nodiscard]] auto asString() const -> std::string;
    [[nodiscard]] constexpr auto getValue() const -> const TokenValue& { return m_value; }
    [[nodiscard]] constexpr auto getStringValue() const -> llvm::StringRef { return m_value.getString(); }
    [[nodiscard]] constexpr auto getRange() const -> llvm::SMRange { return m_range; }
    [[nodiscard]] constexpr auto description() const -> llvm::StringRef { return description(m_kind); }

    // Info about operators
    [[nodiscard]] auto isGeneral() const -> bool;
    [[nodiscard]] auto isLiteral() const -> bool;
    [[nodiscard]] auto isSymbol() const -> bool;
    [[nodiscard]] auto isOperator() const -> bool;
    [[nodiscard]] auto isKeyword() const -> bool;

    [[nodiscard]] auto getPrecedence() const -> int;
    [[nodiscard]] auto isBinary() const -> bool;
    [[nodiscard]] auto isUnary() const -> bool;
    [[nodiscard]] auto isLeftToRight() const -> bool;
    [[nodiscard]] auto isRightToLeft() const -> bool;

    static auto getOperatorType(TokenKind kind) -> OperatorType;

    // Query keyword types

    [[nodiscard]] auto isTypeKeyword() const -> bool;

    // comparisons

    [[nodiscard]] constexpr auto is(TokenKind kind) const -> bool {
        return m_kind == kind;
    }

    [[nodiscard]] constexpr auto isNot(TokenKind kind) const -> bool {
        return m_kind != kind;
    }

    [[nodiscard]] constexpr auto isOneOf(TokenKind k1, TokenKind k2) const -> bool {
        return is(k1) || is(k2);
    }

    template <typename... Ts>
    [[nodiscard]] constexpr auto isOneOf(TokenKind k1, TokenKind k2, Ts... ks) const -> bool {
        return is(k1) || isOneOf(k2, ks...);
    }

private:
    TokenKind m_kind = TokenKind::Invalid;
    TokenValue m_value = TokenValue::NullType {};
    llvm::SMRange m_range;
};

} // namespace lbc
