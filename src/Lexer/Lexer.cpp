//
// Created by Albert Varaksin on 03/07/2020.
//
#include "Lexer.hpp"
#include "Driver/Context.hpp"
#include "Token.hpp"
#include <charconv>
using namespace lbc;

namespace {
using llvm::isAlpha;
using llvm::isDigit;

inline bool isIdentifierChar(char ch) {
    return isAlpha(ch) || isDigit(ch) || ch == '_';
}

inline bool isLineOrFileEnd(char ch) {
    return ch == '\n' || ch == '\r' || ch == '\0';
}

inline llvm::SMRange makeRange(const char* start, const char* end) {
    return { llvm::SMLoc::getFromPointer(start), llvm::SMLoc::getFromPointer(end) };
}

inline std::optional<char> getEscapeChar(char ch) {
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
: m_context{ context },
  m_fileId{ fileID },
  m_hasStmt{ false } {
    const auto* buffer = m_context.getSourceMrg().getMemoryBuffer(m_fileId);
    if (buffer == nullptr) {
        fatalError("Invalid buffer id");
    }
    m_input = buffer->getBufferStart();
    m_end = buffer->getBufferEnd();
    m_eolPos = m_input;
}

Lexer::Lexer(Context& context, unsigned fileID, llvm::SMRange range)
: m_context{ context },
  m_fileId{ fileID },
  m_hasStmt{ false } {
    m_input = range.Start.getPointer();
    m_end = range.End.getPointer();
    m_eolPos = m_input;
}

void Lexer::reset(llvm::SMRange range) {
    m_input = range.Start.getPointer();
    m_end = range.End.getPointer();
    m_eolPos = m_input;
    m_hasStmt = false;
}

void Lexer::next(Token& result) {
    // clang-format off
    while (m_input != m_end) {
        switch (*m_input) {
        case 0:
            return endOfFile(result);
        case '\r':
            m_eolPos = m_input;
            m_input++;
            if (*m_input == '\n') { // CR+LF
                if (m_input < m_end) {
                    m_input++;
                }
            }
            if (m_hasStmt) {
                return endOfStatement(result);
            }
            continue;
        case '\n':
            m_eolPos = m_input;
            m_input++;
            if (m_hasStmt) {
                return endOfStatement(result);
            }
            continue;
        case '\t': case '\v': case '\f': case ' ':
            m_input++;
            continue;
        case '\'':
            skipUntilLineEnd();
            continue;
        case '/':
            if (peekChar() == '\'') {
                skipMultilineComment();
                continue;
            }
            return token(result, TokenKind::Divide);
        case '_':
            if (isIdentifierChar(peekChar())) {
                return identifier(result);
            }
            skipToNextLine();
            continue;
        case '"':
            return stringLiteral(result);
        case '=':
            if (peekChar() == '>') {
                return token(result, TokenKind::LambdaBody, 2);
            }
            return token(result, TokenKind::Assign);
        case ',':
            return token(result, TokenKind::Comma);
        case '.': {
            auto nextCh = peekChar();
            if (nextCh == '.') {
                if (peekChar(2) == '.') {
                    return token(result, TokenKind::Ellipsis, 3);
                }
                break;
            }
            if (isDigit(nextCh)) {
                return numberLiteral(result);
            }
            return token(result, TokenKind::MemberAccess);
        }
        case '(':
            return token(result, TokenKind::ParenOpen);
        case ')':
            return token(result, TokenKind::ParenClose);
        case '[':
            return token(result, TokenKind::BracketOpen);
        case ']':
            return token(result, TokenKind::BracketClose);
        case '+':
            return token(result, TokenKind::Plus);
        case '-':
            return token(result, TokenKind::Minus);
        case '*':
            return token(result, TokenKind::Multiply);
        case '<': {
            auto la = peekChar();
            if (la == '>') {
                return token(result, TokenKind::NotEqual, 2);
            }
            if (la == '=') {
                return token(result, TokenKind::LessOrEqual, 2);
            }
            return token(result, TokenKind::LessThan);
        }
        case '>':
            if (peekChar() == '=') {
                return token(result, TokenKind::GreaterOrEqual, 2);
            }
            return token(result, TokenKind::GreaterThan);
        case '@':
            return token(result, TokenKind::AddressOf);
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return numberLiteral(result);
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
        case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
        case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
        case 'V': case 'W': case 'X': case 'Y': case 'Z':
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
        case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
        case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
        case 'v': case 'w': case 'x': case 'y': case 'z':
            return identifier(result);
        }

        return invalid(result, m_input);
    }
    // clang-format on

    return endOfFile(result);
}

void Lexer::skipUntilLineEnd() {
    // assume m_input[0] != \r || \n
    while (++m_input != m_end) {
        if (isLineOrFileEnd(*m_input)) {
            return;
        }
    }
}

void Lexer::skipToNextLine() {
    // assume m_input != \r || \n
    skipUntilLineEnd();
    if (m_input == m_end) {
        return;
    }

    switch (*m_input) {
    case '\r':
        m_input++;
        if (m_input != m_end && *m_input == '\n') { // CR+LF
            m_input++;
        }
        return;
    case '\n':
        m_input++;
        return;
    }
}

void Lexer::skipMultilineComment() {
    // assume m_input[0] == '/' && m_input[1] == '\''

    m_input++;
    int level = 1;
    while (++m_input != m_end) {
        switch (*m_input) {
        case '\0':
            return;
        case '\'':
            if (peekChar() == '/') {
                m_input++;
                level--;
                if (level == 0) {
                    m_input++;
                    return;
                }
            }
            continue;
        case '/':
            if (peekChar() == '\'') {
                m_input++;
                level++;
            }
        }
    }
}

void Lexer::endOfFile(Token& result) {
    if (m_hasStmt) {
        m_eolPos = m_input;
        return endOfStatement(result);
    }
    result.set(TokenKind::EndOfFile, makeRange(m_input, m_input));
}

void Lexer::endOfStatement(Token& result) {
    m_hasStmt = false;
    result.set(TokenKind::EndOfStmt, makeRange(m_eolPos, m_input));
}

void Lexer::invalid(Token& result, const char* loc) const {
    return result.set(TokenKind::Invalid, makeRange(loc, m_input));
}

void Lexer::stringLiteral(Token& result) {
    // assume m_input[0] == '"'
    m_hasStmt = true;
    const auto* start = m_input;

    std::string literal;
    const auto* begin = m_input + 1;
    while (++m_input != m_end) {
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
                return invalid(result, start);
            }
            continue;
        }
        break;
    }

    result.set(
        TokenKind::StringLiteral,
        makeRange(start, m_input),
        m_context.retainCopy(literal));
}

char Lexer::escape() {
    if (auto ch = getEscapeChar(peekChar())) {
        m_input++;
        return *ch;
    }
    return '\0';
}

void Lexer::token(Token& result, TokenKind kind, int len) {
    // assume m_input[0] == op[0], m_input[len] == next ch
    m_hasStmt = true;
    const auto* start = m_input;
    m_input += len;
    result.set(kind, makeRange(start, m_input));
}

void Lexer::numberLiteral(Token& result) {
    // assume m_input[0] == '.' digit || digit
    m_hasStmt = true;
    const auto* start = m_input;

    bool isFloatingPoint = *m_input == '.';
    if (isFloatingPoint) {
        m_input++;
    }

    do {
        auto ch = *++m_input;
        if (ch == '.') {
            if (isFloatingPoint) {
                return invalid(result, m_input);
            }
            isFloatingPoint = true;
            continue;
        }
        if (isDigit(ch)) {
            continue;
        }
        break;
    } while (m_input != m_end);

    if (isFloatingPoint) {
        std::string const number{ start, m_input };
        std::size_t size{};
        double value = std::stod(number, &size);
        if (size == 0) {
            return invalid(result, start);
        }
        result.set(TokenKind::FloatingPointLiteral, makeRange(start, m_input), value);
        return;
    }

    uint64_t value{};
    constexpr int base10 = 10;
    if (std::from_chars(start, m_input, value, base10).ec != std::errc()) {
        return invalid(result, start);
    }
    result.set(TokenKind::IntegerLiteral, makeRange(start, m_input), value);
}

void Lexer::identifier(Token& result) {
    // assume m_input[0] == '_' || char
    const auto* start = m_input;

    while (++m_input != m_end && isIdentifierChar(*m_input)) {}

    std::string uppercased;
    auto length = std::distance(start, m_input);
    uppercased.reserve(static_cast<size_t>(length));
    std::transform(start, m_input, std::back_inserter(uppercased), llvm::toUpper);

    if (uppercased == "REM") {
        if (m_input != m_end && !isLineOrFileEnd(*m_input)) {
            skipUntilLineEnd();
        }
        next(result);
        return;
    }

    m_hasStmt = true;

    auto range = makeRange(start, m_input);
    auto kind = Token::findKind(uppercased);
    switch (kind) {
    case TokenKind::True:
        return result.set(TokenKind::BooleanLiteral, range, true);
    case TokenKind::False:
        return result.set(TokenKind::BooleanLiteral, range, false);
    case TokenKind::Null:
        return result.set(TokenKind::NullLiteral, range);
    case TokenKind::Identifier:
        return result.set(kind, range, m_context.retainCopy(uppercased));
    default:
        return result.set(kind, range);
    }
}
