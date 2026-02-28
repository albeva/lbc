//
// Created by Albert Varaksin on 13/02/2026.
//
#pragma once
#include "pch.hpp"
namespace lbc {

/**
 * Lightweight value-type wrapper around char, providing a query-based API
 * for character classification. Supports implicit conversion to and from
 * char, so it can be used interchangeably with raw characters.
 *
 * Classification methods encode BASIC lexer semantics. For example,
 * isWhiteSpace() excludes newlines since those are statement terminators.
 */
class [[nodiscard]] Character final {
public:
    /**
     * Implicit conversion from char.
     */
    constexpr Character(const char ch) // NOLINT(*-explicit-conversions)
    : m_char(ch) {}

    /**
     * Return the underlying char value.
     */
    [[nodiscard]] constexpr auto getChar() const -> char { return m_char; }

    /**
     * Implicit conversion to char.
     */
    [[nodiscard]] constexpr operator char() const { return m_char; } // NOLINT(*-explicit-conversions)

    /**
     * Compare two Character values.
     */
    [[nodiscard]] constexpr auto operator==(const Character& other) const -> bool = default;

    /**
     * Compare a Character with a raw char.
     */
    [[nodiscard]] friend constexpr auto operator==(const Character& lhs, const char rhs) -> bool { return lhs.m_char == rhs; }

    /**
     * Check if this character matches any of the given chars.
     */
    [[nodiscard]] constexpr auto isOneOf(const char first, const std::same_as<char> auto&... rest) const -> bool {
        return ((m_char == first) || ... || (m_char == rest));
    }

    /**
     * Check if this is the null terminator, indicating end of input.
     */
    [[nodiscard]] constexpr auto isFileEnd() const -> bool { return m_char == '\0'; }

    /**
     * Check if this is a whitespace character (tab or space). Excludes newlines.
     */
    [[nodiscard]] constexpr auto isWhiteSpace() const -> bool { return isOneOf('\t', ' '); }

    /**
     * Check if this is a line ending character.
     */
    [[nodiscard]] constexpr auto isLineEnd() const -> bool { return isOneOf('\r', '\n'); }

    /**
     * Check if this is a line ending or end of input.
     */
    [[nodiscard]] constexpr auto isFileOrLineEnd() const -> bool { return isOneOf('\0', '\r', '\n'); }

    /**
     * Check if this is an ASCII alphabetic character (a-z, A-Z).
     */
    [[nodiscard]] constexpr auto isAlpha() const -> bool { return (m_char >= 'a' && m_char <= 'z') || (m_char >= 'A' && m_char <= 'Z'); }

    /**
     * Check if this is an ASCII digit (0-9).
     */
    [[nodiscard]] constexpr auto isDigit() const -> bool { return m_char >= '0' && m_char <= '9'; }

    /**
     * Check if this is a valid identifier character (alphanumeric or underscore).
     */
    [[nodiscard]] constexpr auto isIdentifierChar() const -> bool { return isAlpha() || isDigit() || m_char == '_'; }

    /**
     * Check if this is a valid identifier start character (underscore or letter).
     */
    [[nodiscard]] constexpr auto isIdentifierStartChar() const -> bool { return m_char == '_' || isAlpha(); }

    /**
     * Check if this is a visible (printable) ASCII character (>= space).
     */
    [[nodiscard]] constexpr auto isVisible() const -> bool {
        return m_char >= ' ';
    }

    /**
     * Check if this character is a valid escape sequence identifier
     * (i.e., the character following a backslash in a string literal).
     */
    [[nodiscard]] constexpr auto isValidEscape() const -> bool {
        return isOneOf('a', 'b', 'f', 'n', 'r', 't', 'v', '\\', '\'', '"', '0');
    }

private:
    char m_char;
};

} // namespace lbc
