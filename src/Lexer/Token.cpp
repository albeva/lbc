//
// Created by Albert Varaksin on 03/07/2020.
//
#include "Token.h"
#include <charconv>
using namespace lbc;

using std::unordered_set;

namespace {

namespace literals {
#define IMPL_LITERAL(id, kw, ...) constexpr llvm::StringLiteral Str##id{ kw };
    ALL_TOKENS(IMPL_LITERAL)
    KEYWORD_TOKEN_MAP(IMPL_LITERAL)
#undef IMPL_LITERAL
} // namespace literals

/// Map string literal to TokenKind
const llvm::StringMap<TokenKind> keywordsToKind {
#define IMPL_LITERAL(id, ...) { literals::Str##id, TokenKind::id },
    TOKEN_SYMBOLS(IMPL_LITERAL)
    TOKEN_OPERATORS(IMPL_LITERAL)
    TOKEN_KEYWORDS(IMPL_LITERAL)
    ALL_TYPES(IMPL_LITERAL)
#undef IMPL_LITERAL
#define IMPL_KEYWORD_MAP(id, kw, map) { literals::Str##id, TokenKind::map },
    KEYWORD_TOKEN_MAP(IMPL_KEYWORD_MAP)
#undef IMPL_KEYWORD_MAP
};

constexpr std::array kindToDescription {
#define IMPL_LITERAL(id, kw, ...) literals::Str##id,
    ALL_TOKENS(IMPL_LITERAL)
#undef IMPL_LITERAL
};

unordered_set<string> uppercasedIds;
llvm::StringSet literalStrings;

} // namespace

const StringRef& Token::description(TokenKind kind) {
    auto index = static_cast<size_t>(kind);
    return kindToDescription.at(index);
}

unique_ptr<Token> Token::create(const StringRef& lexeme, const llvm::SMLoc& loc) {
    // all identifiers in lb are upper cased
    string uppercased;
    std::transform(lexeme.begin(), lexeme.end(), std::back_inserter(uppercased), llvm::toUpper);

    // is there a matching keyword?
    auto iter = keywordsToKind.find(uppercased);
    if (iter != keywordsToKind.end()) {
        return Token::create(iter->second, iter->first(), loc);
    }

    // tokens store StringRef instances, therefore we need to keep
    // actual string data around. Hence storing all identifiers in a set
    auto entry = uppercasedIds.insert(uppercased);
    return Token::create(TokenKind::Identifier, *entry.first, loc);
}

unique_ptr<Token> Token::create(TokenKind kind, const StringRef& lexeme, const llvm::SMLoc& loc) {
    // literal string may have been processed for escape sequences. Store a local copy
    if (kind == TokenKind::StringLiteral) {
        auto entry = literalStrings.insert(lexeme);
        return make_unique<Token>(kind, entry.first->first(), loc);
    }
    return make_unique<Token>(kind, lexeme, loc);
}

unique_ptr<Token> Token::create(TokenKind kind, const llvm::SMLoc& loc) {
    return create(kind, description(kind), loc);
}

[[nodiscard]] uint64_t Token::getIntegral() const noexcept {
    uint64_t value; // NOLINT
    const int base10 = 10;
    if (std::from_chars(m_lexeme.begin(), m_lexeme.end(), value, base10).ec != std::errc()) {
        fatalError("Failed to parse number: "_t + m_lexeme);
    }
    return value;
}

[[nodiscard]] double Token::getDouble() const noexcept {
    char* end; // NOLINT
    double value = std::strtod(m_lexeme.begin(), &end);
    if (end == m_lexeme.begin()) {
        fatalError("Failed to parse number: "_t + m_lexeme);
    }
    return value;
}

[[nodiscard]] bool Token::getBool() const noexcept {
    if (m_lexeme == "TRUE") { return true; }
    if (m_lexeme == "FALSE") { return false; }
    fatalError("Failed to parse boolean: "_t + m_lexeme);
}


llvm::SMRange Token::range() const {
    if (isGeneral()) {
        return llvm::None;
    }

    auto size = static_cast<ptrdiff_t>(m_lexeme.size());
    if (m_kind == TokenKind::StringLiteral) {
        size += 2;
    }

    const auto* end = m_loc.getPointer() + size; // NOLINT
    return { m_loc, llvm::SMLoc::getFromPointer(end) };
}

const StringRef& Token::description() const {
    if (m_kind == TokenKind::Identifier) {
        return m_lexeme;
    }
    auto index = static_cast<size_t>(m_kind);
    return kindToDescription.at(index);
}

bool Token::isGeneral() const {
#define CASE_GENERAL(id, ...) case TokenKind::id:
    switch (m_kind) {
        TOKEN_GENERAL(CASE_GENERAL)
        return true;
    default:
        return false;
    }
#undef CASE_LITERAL
}

bool Token::isLiteral() const {
#define CASE_LITERAL(id, ...) case TokenKind::id:
    switch (m_kind) {
        TOKEN_LITERALS(CASE_LITERAL)
        return true;
    default:
        return false;
    }
#undef CASE_LITERAL
}

bool Token::isSymbol() const {
#define CASE_SYMBOL(id, ...) case TokenKind::id:
    switch (m_kind) {
        TOKEN_SYMBOLS(CASE_SYMBOL)
        return true;
    default:
        return false;
    }
#undef CASE_SYMBOL
}

bool Token::isOperator() const {
#define CASE_OPERATOR(id, ...) case TokenKind::id:
    switch (m_kind) {
        TOKEN_OPERATORS(CASE_OPERATOR)
        return true;
    default:
        return false;
    }
#undef CASE_OPERATOR
}

bool Token::isKeyword() const {
#define CASE_KEYWORD(id, ...) case TokenKind::id:
    switch (m_kind) {
        TOKEN_KEYWORDS(CASE_KEYWORD)
        return true;
    default:
        return false;
    }
#undef CASE_KEYWORD
}

int Token::getPrecedence() const {
#define CASE_OPERATOR(id, ch, prec, ...) \
    case TokenKind::id:                  \
        return prec;
    switch (m_kind) {
        TOKEN_OPERATORS(CASE_OPERATOR) // NOLINT
    default:
        return 0;
    }
#undef CASE_OPERATOR
}

bool Token::isBinary() const {
    constexpr bool Binary = true; // NOLINT
    constexpr bool Unary = false; // NOLINT
#define CASE_OPERATOR(id, ch, prec, binary, ...) \
    case TokenKind::id:                          \
        return binary;
    switch (m_kind) {
        TOKEN_OPERATORS(CASE_OPERATOR) // NOLINT
    default:
        return false;
    }
#undef CASE_OPERATOR
}

bool Token::isUnary() const {
    constexpr bool Binary = false; // NOLINT
    constexpr bool Unary = true;   // NOLINT
#define CASE_OPERATOR(id, ch, prec, binary, ...) \
    case TokenKind::id:                          \
        return binary;
    switch (m_kind) {
        TOKEN_OPERATORS(CASE_OPERATOR) // NOLINT
    default:
        return false;
    }
#undef CASE_OPERATOR
}

bool Token::isLeftToRight() const {
    constexpr bool Left = true;  // NOLINT
    constexpr bool Right = true; // NOLINT
#define CASE_OPERATOR(id, ch, prec, binary, dir, ...) \
    case TokenKind::id:                               \
        return dir;
    switch (m_kind) {
        TOKEN_OPERATORS(CASE_OPERATOR) // NOLINT
    default:
        return false;
    }
#undef CASE_OPERATOR
}

bool Token::isRightToLeft() const {
    constexpr bool Left = false; // NOLINT
    constexpr bool Right = true; // NOLINT
#define CASE_OPERATOR(id, ch, prec, binary, dir, ...) \
    case TokenKind::id:                               \
        return dir;
    switch (m_kind) {
        TOKEN_OPERATORS(CASE_OPERATOR) // NOLINT
    default:
        return false;
    }
#undef CASE_OPERATOR
}
