//
// Created by Albert Varaksin on 03/07/2020.
//
#pragma once
#include "pch.h"

namespace lbc {

class Context;
class Token;
enum class TokenKind;

class Lexer final {
    NON_COPYABLE(Lexer)
public:
    Lexer(Context& context, unsigned fileID);
    ~Lexer() = default;

    [[nodiscard]] unique_ptr<Token> next();

private:
    [[nodiscard]] unique_ptr<Token> ellipsis();
    [[nodiscard]] unique_ptr<Token> identifier();
    [[nodiscard]] unique_ptr<Token> string();
    [[nodiscard]] unique_ptr<Token> character(TokenKind kind);
    [[nodiscard]] unique_ptr<Token> endOfStatement();
    [[nodiscard]] unique_ptr<Token> endOfFile();
    [[nodiscard]] static unique_ptr<Token> invalid(const char* loc);

    void skipUntilLineEnd();
    bool move();
    [[nodiscard]] bool isValid() const;
    [[nodiscard]] char peek(int ahead = 1) const;

    Context& m_context;
    unsigned m_fileID;
    const llvm::MemoryBuffer* m_buffer;
    const char* m_input;
    char m_char;
    bool m_hasStmt;
    void handleLineEnd();
};

} // namespace lbc
