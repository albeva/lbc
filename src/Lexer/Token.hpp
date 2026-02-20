#pragma once
#include "pch.hpp"

#include "../Symbol/LiteralValue.hpp"
#include "TokenKind.hpp"
namespace lbc {

/**
 * Token represents a single scanned token from the lexer
 */
class Token final {
public:
    /** Default-construct an Invalid sentinel token. */
    constexpr Token()
    : m_kind(TokenKind::Value::Invalid)
    , m_value() { }

    /** Construct a token with a kind, source range, and optional literal value. */
    constexpr Token(
        const TokenKind kind,
        const llvm::SMRange range,
        const LiteralValue value = {}
    )
    : m_kind(kind)
    , m_range(range)
    , m_value(value) {
        assert(range.Start.isValid() && range.End.isValid() && "Token should be created from a valid range");
    }

    /**
     * Return the token kind.
     */
    [[nodiscard]] constexpr auto kind() const -> TokenKind { return m_kind; }

    /**
     * Change token kind. Useful for Minus -> Negate, Assign -> Equal, etc.
     */
    constexpr void changeKind(const TokenKind kind) { m_kind = kind; }

    /**
     * Return the source range covering this token.
     */
    [[nodiscard]] constexpr auto getRange() const -> const llvm::SMRange& { return m_range; }

    /**
     * Return the literal value associated with this token, if any.
     */
    [[nodiscard]] constexpr auto getValue() const -> LiteralValue { return m_value; }

    /**
     * Return a display string for this token. For identifiers and string
     * literals returns the stored value; for numbers returns the raw
     * lexeme; for everything else returns the token kind string.
     */
    [[nodiscard]] auto string() const -> llvm::StringRef;

    /**
     * Return the raw source text covered by this token's range.
     */
    [[nodiscard]] auto lexeme() const -> llvm::StringRef;

private:
    TokenKind m_kind;
    llvm::SMRange m_range;
    LiteralValue m_value;
};

} // namespace lbc

/**
 * Support using Token with std::print and std::format.
 */
template <>
struct std::formatter<lbc::Token, char> final {
    constexpr static auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const lbc::Token& value, auto& ctx) const {
        return std::format_to(ctx.out(), "{}", std::string_view(value.string()));
    }
};
