//
// Created by Albert Varaksin on 12/02/2026.
//
#include "Lexer.hpp"
#include "Driver/Context.hpp"
using namespace lbc;

Lexer::Lexer(Context& context, unsigned id)
: m_context(context)
, m_id(id)
, m_current(context.getSourceMgr().getMemoryBuffer(id)->getBufferStart())
, m_end(context.getSourceMgr().getMemoryBuffer(id)->getBufferStart()) {
}

auto Lexer::next() -> Token {
    (void)this;
    return Token { TokenKind::EndOfFile, {} };
}
