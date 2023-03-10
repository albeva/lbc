//
// Created by Albert on 03/03/2022.
//
#pragma once
#include "pch.hpp"
#include "Token.hpp"
#include "TokenSource.hpp"

namespace lbc {
class TokenProvider final : public TokenSource {
public:
    TokenProvider(unsigned int fileId, std::vector<Token>&& tokens)
    : m_fileId{ fileId }, m_tokens{ std::move(tokens) } {}

    unsigned int getFileId() override { return m_fileId; }
    void next(Token& result) override;
    void peek(Token& result) override;
    void reset() { m_index = 0; }

    llvm::SMRange getRange() const ;

private:
    unsigned int m_fileId;
    size_t m_index = 0;
    std::vector<Token> m_tokens;
};

} // namespace lbc
