//
// Created by Albert on 03/03/2022.
//
#include "TokenProvider.hpp"
using namespace lbc;

void TokenProvider::next(Token& result) {
    if (m_index < m_tokens.size()) {
        result = m_tokens[m_index++];
        return;
    }
    result.set(TokenKind::EndOfStmt, {});
}

void TokenProvider::peek(Token& result) {
    if (m_index < m_tokens.size()) {
        result = m_tokens[m_index];
        return;
    }
    result.set(TokenKind::EndOfStmt, {});
}

llvm::SMRange TokenProvider::getRange() const noexcept {
    if (m_tokens.empty()) {
        return {};
    }
    return { m_tokens.front().range().Start, m_tokens.back().range().End };
}
