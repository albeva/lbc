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

    [[nodiscard]] auto getFileId() const -> unsigned int { return m_fileId; }
    [[nodiscard]] inline auto getBuffer() const -> const llvm::MemoryBuffer*;
    void reset(llvm::SMLoc loc);

    void next(Token& result);
    void peek(Token& result) const { Lexer(*this).next(result); }

private:
    void skipUntilLineEnd();
    void skipToNextLine();
    void skipMultilineComment();

    void endOfFile(Token& result);
    void endOfStatement(Token& result);
    void invalid(Token& result, const char* loc) const;
    void stringLiteral(Token& result);
    void token(Token& result, TokenKind kind, int len = 1);
    void numberLiteral(Token& result);
    void identifier(Token& result);

    [[nodiscard]] auto escape() -> char;

    const char* m_input;
    const char* m_eolPos;
    Context* m_context;
    bool m_hasStmt;
    unsigned int m_fileId;
};

} // namespace lbc
