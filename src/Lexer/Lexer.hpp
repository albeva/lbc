//
// Created by Albert Varaksin on 03/07/2020.
//
#pragma once
#include "pch.hpp"

namespace lbc {
class Token;
class Context;
enum class TokenKind;

class Lexer final {
public:
    NO_COPY_AND_MOVE(Lexer)

    Lexer(Context& context, unsigned fileID);
    Lexer(Context& context, unsigned fileID, llvm::SMRange range);

    void reset(llvm::SMRange range);

    [[nodiscard]] unsigned int getFileId() const { return m_fileId; }
    void next(Token& result);
    void peek(Token& result);

private:
    void skipUntilLineEnd();
    void skipToNextLine();
    void skipMultilineComment();

    void endOfFile(Token& result);
    void endOfStatement(Token& result);
    void invalid(Token& result, const char* loc) const;
    void stringLiteral(Token& result);
    [[nodiscard]] char escape();
    void token(Token& result, TokenKind kind, int len = 1);
    void numberLiteral(Token& result);
    void identifier(Token& result);
    [[nodiscard]] char peekChar(size_t offset = 1) const;
    void clampInput() { if (m_input > m_end) { m_input = m_end; } }

    Context& m_context;
    unsigned int m_fileId;
    const char* m_input;
    const char* m_end;
    const char* m_eolPos;
    bool m_hasStmt;
};

} // namespace lbc
