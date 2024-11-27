//
// Created by Albert Varaksin on 03/07/2020.
//
#pragma once
#include "pch.hpp"

namespace lbc {
class Token;
class Context;
enum class TokenKind : std::uint8_t;

class Lexer final {
public:
    Lexer(Context& context, unsigned fileID);
    Lexer(Context& context, unsigned fileID, llvm::SMLoc loc);

    void reset(llvm::SMLoc loc);

    [[nodiscard]] auto getFileId() const -> unsigned int { return m_fileId; }

    void next(Token& result);

    void peek(Token& result) const {
        Lexer(*this).next(result);
    }

private:

    void skipUntilLineEnd();
    void skipToNextLine();
    void skipMultilineComment();

    void endOfFile(Token& result);
    void endOfStatement(Token& result);
    void invalid(Token& result, const char* loc) const;
    void stringLiteral(Token& result);
    [[nodiscard]] auto escape() -> char;
    void token(Token& result, TokenKind kind, int len = 1);
    void numberLiteral(Token& result);
    void identifier(Token& result);

    [[nodiscard]] auto peekChar(std::size_t offset = 1) const -> char;

    Context* m_context;
    unsigned int m_fileId;
    const char* m_input;
    const char* m_end;
    const char* m_eolPos;
    bool m_hasStmt;
};

} // namespace lbc
