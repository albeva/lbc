#pragma once
#include "pch.hpp"
#include "Lexer/Tokens.inc"
#include "Utilities/LiteralValue.hpp"
namespace lbc {

/**
 * Token represents a single scanned token from the lexer
 */
class Token final {
public:
    explicit constexpr Token(
        const TokenKind kind,
        const llvm::SMRange range,
        const LiteralValue value = {}
    )
    : m_kind(kind)
    , m_range(range)
    , m_value(value) { }

    [[nodiscard]] constexpr auto kind() const -> TokenKind { return m_kind; }
    [[nodiscard]] constexpr auto getRange() const -> llvm::SMRange { return m_range; }
    [[nodiscard]] constexpr auto getValue() const -> LiteralValue { return m_value; }

private:
    TokenKind m_kind;
    llvm::SMRange m_range;
    LiteralValue m_value;
};

} // namespace lbc
