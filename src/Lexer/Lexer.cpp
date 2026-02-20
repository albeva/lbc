//
// Created by Albert Varaksin on 12/02/2026.
//
#include "Lexer.hpp"
#include "Driver/Context.hpp"
using namespace lbc;

namespace {
const llvm::StringMap<TokenKind> kKeywords = [] static {
    llvm::StringMap<TokenKind> keywords {};
    for (const auto& kind : TokenKind::allKeywords()) {
        keywords.try_emplace(kind.string(), kind);
    }
    for (const auto& kind : TokenKind::allOperatorKeywords()) {
        keywords.try_emplace(kind.string(), kind);
    }
    for (const auto& kind : TokenKind::allTypes()) {
        keywords.try_emplace(kind.string(), kind);
    }
    return keywords;
}();
} // namespace

Lexer::Lexer(Context& context, unsigned id)
: m_context(context)
, m_id(id)
, m_start(context.getSourceMgr().getMemoryBuffer(id)->getBufferStart())
, m_input(m_start)
, m_hasStatement(false) {
}

auto Lexer::next() -> DiagResult<Token> {
    while (true) {
        switch (m_input.current().getChar()) {
        case '\0':
            return endOfFile();
        case '\r':
            m_start = m_input;
            m_input.advance();
            if (m_input.current() == '\n') {
                m_input.advance();
            }
            if (m_hasStatement) {
                return endOfStmt();
            }
            continue;
        case '\n':
            m_start = m_input;
            m_input.advance();
            if (m_hasStatement) {
                return endOfStmt();
            }
            continue;
        case ' ':
        case '\t':
            m_input.advance();
            continue;
        case '\'':
            skipUntilLineEnd();
            continue;
        case '/':
            if (m_input.peek() == '\'') {
                skipMultilineComment();
                continue;
            }
            return make(TokenKind::Divide);
        case '_':
            if (m_input.peek().isIdentifierChar()) {
                return identifier();
            }
            skipToNextLine();
            continue;
        case '=':
            return make(TokenKind::Assign);
        case ',':
            return make(TokenKind::Comma);
        case '.': {
            const auto la = m_input.peek();
            if (la == '.') {
                if (m_input.peek(2) == '.') {
                    return make(TokenKind::Ellipsis, 3);
                }
                return invalid();
            }
            if (la.isDigit()) {
                return numberLiteral();
            }
            return make(TokenKind::MemberAccess);
        }
        case '(':
            return make(TokenKind::ParenOpen);
        case ')':
            return make(TokenKind::ParenClose);
        case '[':
            return make(TokenKind::BracketOpen);
        case ']':
            return make(TokenKind::BracketClose);
        case '+':
            return make(TokenKind::Plus);
        case '-':
            if (m_input.peek() == '>') {
                return make(TokenKind::PointerAccess, 2);
            }
            return make(TokenKind::Minus);
        case '*':
            return make(TokenKind::Multiply);
        case '<': {
            const auto la = m_input.peek();
            if (la == '>') {
                return make(TokenKind::NotEqual, 2);
            }
            if (la == '=') {
                return make(TokenKind::LessOrEqual, 2);
            }
            return make(TokenKind::LessThan);
        }
        case '>':
            if (m_input.peek() == '=') {
                return make(TokenKind::GreaterOrEqual, 2);
            }
            return make(TokenKind::GreaterThan);
        case '@':
            return make(TokenKind::AddressOf);
            // clang-format off
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            // clang-format on
            return numberLiteral();
        case '"':
            return stringLiteral();
            // clang-format off
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
        case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
        case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
        case 'V': case 'W': case 'X': case 'Y': case 'Z':
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
        case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
        case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
        case 'v': case 'w': case 'x': case 'y': case 'z':
            // clang-format on
            return identifier();
        default:
            m_input.advance();
            return invalid();
        }
    }
}

auto Lexer::peek() -> DiagResult<Token> {
    const ValueRestorer restorer { m_start, m_input, m_hasStatement };
    return next();
}

// ------------------------------------
// Token factories
// ------------------------------------

auto Lexer::invalid() -> DiagError {
    return diag(diagnostics::invalid(), range().Start, range());
}

auto Lexer::endOfFile() -> Token {
    if (m_hasStatement) {
        return endOfStmt();
    }
    return Token { TokenKind::EndOfFile, range() };
}

auto Lexer::endOfStmt() -> Token {
    m_hasStatement = false;
    return Token { TokenKind::EndOfStmt, range() };
}

auto Lexer::make(const TokenKind kind, const std::size_t len) -> Token {
    m_hasStatement = true;
    m_start = m_input;
    m_input.advance(len);
    return Token { kind, range() };
}

auto Lexer::token(const TokenKind kind, const LiteralValue value) -> Token {
    m_hasStatement = true;
    return Token { kind, range(), value };
}

// ------------------------------------
// Skip sequences
// ------------------------------------

void Lexer::skipUntilLineEnd() {
    m_input.advanceWhileNot(&Character::isFileOrLineEnd);
}

void Lexer::skipMultilineComment() {
    // assume m_input[0] == '/' && m_input[1] == '\''
    assert(m_input.peek(0) == '/');
    assert(m_input.peek(1) == '\'');
    m_input.advance(2);

    int level = 1;
    m_input.advanceWhile([&](const auto ch) {
        switch (ch) {
        case '\0':
            return false;
        case '\'':
            if (m_input.peek() == '/') {
                m_input.advance();
                --level;
                if (level == 0) {
                    m_input.advance();
                    return false;
                }
            }
            return true;
        case '/':
            if (m_input.peek() == '\'') {
                m_input.advance();
                ++level;
            }
            return true;
        default:
            return true;
        }
    });
}

void Lexer::skipToNextLine() {
    m_input.advanceWhile([&](const auto ch) {
        switch (ch) {
        case '\0':
            return false;
        case '\r':
            m_input.advance();
            if (m_input.current() == '\n') {
                m_input.advance();
            }
            return false;
        case '\n':
            m_input.advance();
            return false;
        default:
            return true;
        }
    });
}

// ------------------------------------
// Token lexers
// ------------------------------------

auto Lexer::identifier() -> DiagResult<Token> {
    // assume m_input[0] == '_' || m_input[0].isAlpha()
    assert(m_input.current().isIdentifierStartChar() && "Unexpected identifier start");

    m_start = m_input;
    m_input.advance();
    m_input.advanceWhile(&Character::isIdentifierChar);

    // fetch string literal and uppercase it in-place
    m_buffer.clear();
    std::transform(m_start.data(), m_input.data(), std::back_inserter(m_buffer), [](const char ch) {
        if (ch >= 'a' && ch <= 'z') {
            return static_cast<char>(ch - ('a' - 'A'));
        }
        return ch;
    });

    // Is a keyword?
    if (const auto iter = kKeywords.find(m_buffer); iter != kKeywords.end()) {
        switch (iter->second.value()) {
        case TokenKind::True:
            return token(TokenKind::BooleanLiteral, LiteralValue::from(true));
        case TokenKind::False:
            return token(TokenKind::BooleanLiteral, LiteralValue::from(false));
        case TokenKind::Null:
            return token(TokenKind::NullLiteral);
        default:
            return token(iter->second);
        }
    }

    // Is an identifier
    return token(TokenKind::Identifier, LiteralValue::from(m_context.retain(m_buffer)));
}

auto Lexer::stringLiteral() -> DiagResult<Token> {
    // assume m_input[0] == '"'
    assert(m_input.current() == '"');
    m_start = m_input;
    m_input.advance();

    bool hasError = false;
    m_input.advanceWhile([&](const Character ch) {
        if (ch == '\\') {
            if (m_input.peek().isValidEscape()) {
                m_input.advance();
                return true;
            }
            hasError = true;
            return false;
        }
        if (!ch.isVisible()) {
            hasError = true;
            return false;
        }
        return !(ch == '"' || ch.isFileOrLineEnd());
    });

    // unclosed string?
    if (hasError || m_input.current() != '"') {
        return diag(diagnostics::unterminatedString(), m_start.loc(), range());
    }

    const auto str = m_start.next().stringTo(m_input);
    m_input.advance();
    return token(TokenKind::StringLiteral, LiteralValue::from(str));
}

auto Lexer::numberLiteral() -> DiagResult<Token> {
    // TODO: Proper integral and FP number lexing

    // assume m_input[0] == '.' || m_input[0].isDigit()
    assert(m_input.current() == '.' || m_input.current().isDigit());
    bool isFloat = m_input.current() == '.';

    m_start = m_input;
    m_input.advance();

    bool hasError = false;
    m_input.advanceWhile([&](const auto& ch) {
        if (ch.isDigit()) {
            return true;
        }
        if (ch == '.') {
            if (!isFloat) {
                isFloat = true;
                return true;
            }
            hasError = true;
            return false;
        }
        if (ch.isIdentifierStartChar()) {
            hasError = true;
        }
        return false;
    });

    if (!hasError) {
        if (isFloat) {
            if (const auto value = number<double>()) {
                return token(TokenKind::FloatLiteral, LiteralValue::from(*value));
            }
        } else if (const auto value = number<std::uint64_t>()) {
            return token(TokenKind::IntegerLiteral, LiteralValue::from(*value));
        }
    }
    return diag(diagnostics::invalidNumber(), m_start.loc(), range());
}
