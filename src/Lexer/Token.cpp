#include "Token.hpp"
using namespace lbc;

auto Token::string() const -> llvm::StringRef {
    switch (m_kind.value()) {
    case TokenKind::Identifier:
    case TokenKind::StringLiteral:
        return std::get<llvm::StringRef>(m_value);
    case TokenKind::IntegerLiteral:
    case TokenKind::FloatLiteral: {
        return lexeme();
    }
    default:
        return m_kind.string();
    }
}

auto Token::lexeme() const -> llvm::StringRef {
    const char* start = m_range.Start.getPointer();
    const char* end = m_range.End.getPointer();
    const auto len = static_cast<std::size_t>(end - start);
    return { start, len };
}
