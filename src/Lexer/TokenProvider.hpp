//
// Created by Albert on 03/03/2022.
//
#pragma once
#include "Token.hpp"
#include "TokenSource.hpp"

namespace lbc {
class TokenProvider final : public TokenSource {
public:
    TokenProvider(unsigned int fileId, std::vector<Token>& tokens) noexcept
    : m_fileId{ fileId }, m_tokens{ tokens } {}

    unsigned int getFileId() override { return m_fileId; }
    void next(Token& result) override;
    void peek(Token& result) override;

private:
    unsigned int m_fileId;
    size_t m_index = 0;
    std::vector<Token>& m_tokens;
};

} // namespace lbc
