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
    Lexer() = delete;
    Lexer(Lexer&&) = delete;
    Lexer& operator=(const Lexer&) = delete;
    Lexer& operator=(Lexer&&) = delete;

    Lexer(Context& context, unsigned fileID);
    Lexer(Context& context, unsigned fileID, llvm::SMRange range);

    void reset(llvm::SMRange range);

    [[nodiscard]] unsigned int getFileId() const { return m_fileId; }
    void next(Token& result);
    void peek(Token& result);

private:
    Lexer(const Lexer&) = default;

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

    [[nodiscard]] inline char peekChar(std::size_t offset = 1) const {
        const char* peek = m_input + offset;
        if (peek < m_end) {
            return *peek;
        }
        return '\0';
    }

    inline void clampInput() {
        if (m_input > m_end) {
            m_input = m_end;
        }
    }

    Context& m_context;
    unsigned int m_fileId;
    const char* m_input;
    const char* m_end;
    const char* m_eolPos;
    bool m_hasStmt;
};

} // namespace lbc
