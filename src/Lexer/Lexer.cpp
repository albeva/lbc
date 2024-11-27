//
// Created by Albert Varaksin on 03/07/2020.
//
#include "Lexer.hpp"
#include "Driver/Context.hpp"
#include "Token.hpp"
#include <charconv>
using namespace lbc;

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic, readability-avoid-return-with-void-value)

namespace {
using llvm::isAlpha;
using llvm::isDigit;

inline auto isIdentifierChar(char ch) -> bool {
    return isAlpha(ch) || isDigit(ch) || ch == '_';
}

inline auto isLineOrFileEnd(char ch) -> bool {
    return ch == '\n' || ch == '\r' || ch == '\0';
}

inline auto makeRange(const char* start, const char* end) -> llvm::SMRange {
    return { llvm::SMLoc::getFromPointer(start), llvm::SMLoc::getFromPointer(end) };
}

inline auto getEscapeChar(char ch) -> std::optional<char> {
    switch (ch) {
    case 'a':
        return '\a';
    case 'b':
        return '\b';
    case 'f':
        return '\f';
    case 'n':
        return '\n';
    case 'r':
        return '\r';
    case 't':
        return '\t';
    case 'v':
        return '\v';
    case '\\':
        return '\\';
    case '\'':
        return '\'';
    case '"':
        return '"';
    case '\?':
        return '?';
    case '0':
        return '\0';
    default:
        return std::nullopt;
    }
}
} // namespace

Lexer::Lexer(Context& context, unsigned fileID)
: m_context{ &context },
  m_hasStmt{ false },
  m_fileId{ fileID } {
    const auto* buffer = getBuffer();
    if (buffer == nullptr) {
        fatalError("Invalid buffer id");
    }
    m_input = buffer->getBufferStart();
    m_eolPos = m_input;
}

auto Lexer::getBuffer() const -> const llvm::MemoryBuffer* {
    return m_context->getSourceMrg().getMemoryBuffer(m_fileId);
}

void Lexer::reset(llvm::SMLoc loc) {
    m_input = loc.getPointer();
    assert(m_input >= getBuffer()->getBufferStart() && m_input < getBuffer()->getBufferEnd() && "Invalid source location");
    m_eolPos = m_input;
    m_hasStmt = false;
}

auto Lexer::next() -> Token {
    // clang-format off
    while (true) {
        switch (*m_input) {
        case 0:
            return endOfFile();
        case '\r':
            m_eolPos = m_input;
            m_input++;
            if (*m_input == '\n') { // CR+LF
                m_input++;
            }
            if (m_hasStmt) {
                return endOfStatement();
            }
            continue;
        case '\n':
            m_eolPos = m_input;
            m_input++;
            if (m_hasStmt) {
                return endOfStatement();
            }
            continue;
        case '\t': case '\v': case '\f': case ' ':
            m_input++;
            continue;
        case '\'':
            skipUntilLineEnd();
            continue;
        case '/':
            if (m_input[1] == '\'') {
                skipMultilineComment();
                continue;
            }
            return token(TokenKind::Divide);
        case '_':
            if (isIdentifierChar(m_input[1])) {
                return identifier();
            }
            skipToNextLine();
            continue;
        case '"':
            return stringLiteral();
        case '=':
            if (m_input[1] == '>') {
                return token(TokenKind::LambdaBody, 2);
            }
            return token(TokenKind::AssignOrEqual);
        case ',':
            return token(TokenKind::CommaOrConditionAnd);
        case '.': {
            auto nextCh = m_input[1];
            if (nextCh == '.') {
                if (m_input[2] == '.') {
                    return token(TokenKind::Ellipsis, 3);
                }
                break;
            }
            if (isDigit(nextCh)) {
                return numberLiteral();
            }
            return token(TokenKind::MemberAccess);
        }
        case '(':
            return token(TokenKind::ParenOpen);
        case ')':
            return token(TokenKind::ParenClose);
        case '[':
            return token(TokenKind::BracketOpen);
        case ']':
            return token(TokenKind::BracketClose);
        case '+':
            return token(TokenKind::Plus);
        case '-':
            return token(TokenKind::MinusOrNegate);
        case '*':
            return token(TokenKind::MultiplyOrDereference);
        case '<': {
            auto la = m_input[1];
            if (la == '>') {
                return token(TokenKind::NotEqual, 2);
            }
            if (la == '=') {
                return token(TokenKind::LessOrEqual, 2);
            }
            return token(TokenKind::LessThan);
        }
        case '>':
            if (m_input[1] == '=') {
                return token(TokenKind::GreaterOrEqual, 2);
            }
            return token(TokenKind::GreaterThan);
        case '@':
            return token(TokenKind::AddressOf);
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return numberLiteral();
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
        case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
        case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
        case 'V': case 'W': case 'X': case 'Y': case 'Z':
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
        case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
        case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
        case 'v': case 'w': case 'x': case 'y': case 'z':
            return identifier();
        default:
            return invalid(m_input);
        }
    }
    // clang-format on
}

void Lexer::skipUntilLineEnd() {
    while (!isLineOrFileEnd(*m_input)) {
        m_input++;
    }
}

void Lexer::skipToNextLine() {
    // assume m_input != \r || \n
    skipUntilLineEnd();

    switch (*m_input) {
    case '\0':
        return;
    case '\r':
        m_input++;
        if (*m_input == '\n') { // CR+LF
            m_input++;
        }
        return;
    case '\n':
        m_input++;
        return;
    default:
        return;
    }
}

void Lexer::skipMultilineComment() {
    // assume m_input[0] == '/' && m_input[1] == '\''

    m_input++;
    int level = 1;
    while (true) {
        m_input++;
        switch (*m_input) {
        case '\0':
            return;
        case '\'':
            if (m_input[1] == '/') {
                m_input++;
                level--;
                if (level == 0) {
                    m_input++;
                    return;
                }
            }
            break;
        case '/':
            if (m_input[1] == '\'') {
                m_input++;
                level++;
            }
            break;
        default:
            break;
        }
    }
}

auto Lexer::endOfFile() -> Token {
    if (m_hasStmt) {
        m_eolPos = m_input;
        return endOfStatement();
    }
    return { TokenKind::EndOfFile, makeRange(m_input, m_input) };
}

auto Lexer::endOfStatement() -> Token {
    m_hasStmt = false;
    return { TokenKind::EndOfStmt, makeRange(m_eolPos, m_input) };
}

auto Lexer::invalid(const char* loc) const -> Token {
    return { TokenKind::Invalid, makeRange(loc, m_input) };
}

auto Lexer::stringLiteral() -> Token {
    // assume m_input[0] == '"'
    m_hasStmt = true;
    const auto* start = m_input;

    std::string literal;
    const auto* begin = m_input + 1;
    while (true) {
        m_input++;
        auto ch = *m_input;
        switch (ch) {
        case '\t':
            continue;
        case '\\':
            if (begin < m_input) {
                literal.append(begin, m_input);
            }
            literal += escape();
            begin = m_input + 1;
            continue;
        case '"':
            if (begin < m_input) {
                literal.append(begin, m_input);
            }
            m_input++;
            break;
        default:
            constexpr char visibleFrom = 32;
            if (ch < visibleFrom) {
                return invalid(start);
            }
            continue;
        }
        break;
    }

    return {
        TokenKind::StringLiteral,
        makeRange(start, m_input),
        m_context->retainCopy(literal)
    };
}

auto Lexer::escape() -> char {
    if (auto ch = getEscapeChar(m_input[1])) {
        m_input++;
        return *ch;
    }
    return '\0';
}

auto Lexer::token(TokenKind kind, int len) -> Token {
    // assume m_input[0] == op[0], m_input[len] == next ch
    m_hasStmt = true;
    const auto* start = m_input;
    m_input += len;
    return { kind, makeRange(start, m_input) };
}

auto Lexer::numberLiteral() -> Token {
    // assume m_input[0] == '.' digit || digit
    m_hasStmt = true;
    const auto* start = m_input;

    bool isFloatingPoint = *m_input == '.';
    if (isFloatingPoint) {
        m_input++;
    }
    m_input++;

    while (true) {
        auto ch = *m_input;
        if (ch == '.') {
            if (isFloatingPoint) {
                return invalid(m_input);
            }
            isFloatingPoint = true;
            m_input++;
            continue;
        }
        if (isDigit(ch)) {
            m_input++;
            continue;
        }
        break;
    }

    if (isFloatingPoint) {
        std::string const number{ start, m_input };
        std::size_t size{};
        double value = std::stod(number, &size);
        if (size == 0) {
            return invalid(start);
        }
        return { TokenKind::FloatingPointLiteral, makeRange(start, m_input), value };
    }

    uint64_t value{};
    constexpr int base10 = 10;
    if (std::from_chars(start, m_input, value, base10).ec != std::errc()) {
        return invalid(start);
    }
    return { TokenKind::IntegerLiteral, makeRange(start, m_input), value };
}

auto Lexer::identifier() -> Token {
    // assume m_input[0] == '_' || char
    const auto* start = m_input;
    m_input++;

    // lex identifier body
    while (isIdentifierChar(*m_input)) {
        m_input++;
    }

    // get uppercased string
    auto length = static_cast<size_t>(std::distance(start, m_input));
    std::string uppercased;
    uppercased.reserve(length);
    std::transform(start, m_input, std::back_inserter(uppercased), llvm::toUpper);

    // determine token kind
    auto kind = Token::findKind(uppercased);

    // is it a REM single line comment?
    if (kind == TokenKind::Rem) {
        skipUntilLineEnd();
        return next();
    }

    // either a keyword or an identifier
    auto range = makeRange(start, m_input);
    m_hasStmt = true;

    switch (kind) {
    case TokenKind::True:
        return { TokenKind::BooleanLiteral, range, true };
    case TokenKind::False:
        return { TokenKind::BooleanLiteral, range, false };
    case TokenKind::Null:
        return { TokenKind::NullLiteral, range };
    case TokenKind::Identifier:
        return { kind, range, m_context->retainCopy(uppercased) };
    default:
        return { kind, range };
    }
}

// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic, readability-avoid-return-with-void-value)
