//
// Created by Albert Varaksin on 03/07/2020.
//
#pragma once
#include "pch.hpp"
#include "Token.hpp"

namespace lbc {
class Token;
class Context;
enum class TokenKind : std::uint8_t;

class Lexer final {
public:
    Lexer(Context& context, unsigned fileID, llvm::SMLoc loc = {});

    [[nodiscard]] auto getFileId() const -> unsigned int { return m_fileId; }
    [[nodiscard]] inline auto getBuffer() const -> const llvm::MemoryBuffer*;
    void reset(llvm::SMLoc loc);

    auto next() -> Token;

private:
    void skipUntilLineEnd();
    void skipToNextLine();
    void skipMultilineComment();

    auto endOfFile() -> Token;
    auto endOfStatement() -> Token;
    auto invalid(const char* loc) const -> Token;
    auto stringLiteral() -> Token;
    auto token(TokenKind kind, int len = 1) -> Token;
    auto numberLiteral() -> Token;
    auto identifier() -> Token;

    [[nodiscard]] auto escape() -> char;

    const char* m_input;
    const char* m_eolPos;
    Context* m_context;
    bool m_hasStmt;
    unsigned int m_fileId;
};

} // namespace lbc
