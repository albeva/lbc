//
// Created by Albert Varaksin on 03/07/2020.
//
#include "Token.hpp"
using namespace lbc;

namespace {
enum class Category : std::uint8_t {
    General,
    Literal,
    Symbol,
    Operator,
    Keyword,
    Type
};

enum class OpType : std::uint8_t {
    Unknown,
    Unary,
    Binary
};

enum class OpAssociativity : std::uint8_t {
    Unknown,
    Left,
    Right
};

struct TokenDef final {
    Category category {};
    llvm::StringLiteral str;
    int precedence = 0;
    OpType type = OpType::Unknown;
    OpAssociativity assoc = OpAssociativity::Unknown;
    OperatorType kind = OperatorType::Arithmetic;
};

// clang-format off
namespace literals {
    #define IMPL_LITERAL(ID, kw, ...) constexpr llvm::StringLiteral ID { kw };
    ALL_TOKENS(IMPL_LITERAL)
    #undef IMPL_LITERAL
}

// Map string literal to TokenKind
const llvm::StringMap<TokenKind> keywordsToKind {
    #define IMPL_LITERAL(ID, ...) { literals::ID, TokenKind::ID },
    TOKEN_KEYWORDS(IMPL_LITERAL)
    ALL_TYPES(IMPL_LITERAL)
    TOKEN_OPERATOR_KEYWORD_MAP(IMPL_LITERAL)
    #undef IMPL_LITERAL
};

// array of token definitions
constexpr const std::array tokenDefs {
    #define GENERAL(ID, STR, ...) TokenDef { Category::General, literals::ID },
    #define LITERAL(ID, STR, ...) TokenDef { Category::Literal, literals::ID },
    #define SYMBOL(ID, STR, ...) TokenDef { Category::Symbol, literals::ID },
    #define KEYWORD(ID, STR, ...) TokenDef { Category::Keyword, literals::ID },
    #define TYPE(ID, STR, ...) TokenDef { Category::Type, literals::ID },
    #define OPERATOR(ID, STR, PREC, TYPE, ASSOC, CAT) TokenDef { Category::Operator, literals::ID, PREC, OpType::TYPE, OpAssociativity::ASSOC, OperatorType::CAT },
    TOKEN_GENERAL(GENERAL)
    TOKEN_LITERALS(LITERAL)
    TOKEN_SYMBOLS(SYMBOL)
    TOKEN_OPERATORS(OPERATOR)
    TOKEN_KEYWORDS(KEYWORD)
    ALL_TYPES(TYPE)
    #undef GENERAL
    #undef LITERAL
    #undef SYMBOL
    #undef KEYWORD
    #undef TYPE
    #undef OPERATOR
};
// clang-format on

constexpr auto getDef(TokenKind kind) -> const TokenDef& {
    const auto index = static_cast<size_t>(kind);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    return tokenDefs[index];
}

} // namespace

auto Token::description(const TokenKind kind) -> llvm::StringRef {
    return getDef(kind).str;
}

auto Token::findKind(const llvm::StringRef str) -> TokenKind {
    if (const auto iter = keywordsToKind.find(str); iter != keywordsToKind.end()) {
        return iter->second;
    }
    return TokenKind::Identifier;
}

auto Token::lexeme() const -> llvm::StringRef {
    const auto* start = m_range.Start.getPointer();
    const auto* end = m_range.End.getPointer();
    return { start, static_cast<size_t>(std::distance(start, end)) }; // NOLINT
}

auto Token::asString() const -> std::string {
    static constexpr auto visitor = lbc::Visitor {
        [](TokenValue::NullType /*value*/) {
            return literals::Null.str();
        },
        [](const TokenValue::StringType value) {
            return value.str();
        },
        [](const TokenValue::IntegralType value) {
            return std::to_string(value);
        },
        [](const TokenValue::FloatingPointType value) {
            return std::to_string(value);
        },
        [](const bool value) {
            return (value ? literals::True : literals::False).str();
        }
    };

    if (isLiteral() || m_kind == TokenKind::Identifier) {
        return std::visit(visitor, m_value);
    }
    return description().str();
}

auto Token::isGeneral() const -> bool {
    return getDef(m_kind).category == Category::General;
}

auto Token::isLiteral() const -> bool {
    return getDef(m_kind).category == Category::Literal;
}

auto Token::isSymbol() const -> bool {
    return getDef(m_kind).category == Category::Symbol;
}

auto Token::isOperator() const -> bool {
    return getDef(m_kind).category == Category::Operator;
}

auto Token::isKeyword() const -> bool {
    const auto cat = getDef(m_kind).category;
    return cat == Category::Keyword || cat == Category::Type;
}

auto Token::getPrecedence() const -> int {
    return getDef(m_kind).precedence;
}

auto Token::isBinary() const -> bool {
    return getDef(m_kind).type == OpType::Binary;
}

auto Token::isUnary() const -> bool {
    return getDef(m_kind).type == OpType::Unary;
}

auto Token::isLeftToRight() const -> bool {
    return getDef(m_kind).assoc == OpAssociativity::Left;
}

auto Token::isRightToLeft() const -> bool {
    return getDef(m_kind).assoc == OpAssociativity::Right;
}

auto Token::getOperatorType(const TokenKind kind) -> OperatorType {
    return getDef(kind).kind;
}

auto Token::isTypeKeyword() const -> bool {
    return getDef(m_kind).category == Category::Type;
}
