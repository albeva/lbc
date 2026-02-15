//
// Created by Albert Varaksin on 12/02/2026.
//
#include "Parser.hpp"
using namespace lbc;

Parser::Parser(Context& context, const unsigned id)
: m_context(context)
, m_lexer(context, id)
, m_token(m_lexer.next()) { }

Parser::~Parser() = default;

void Parser::parse() {
    while (true) {
        std::println("'{}'", m_token);
        m_token = m_lexer.next();
        if (m_token.kind().isOneOf(TokenKind::EndOfFile, TokenKind::Invalid)) {
            break;
        }
    }
}

void Parser::panic(const DiagMessage message) {
    // TODO: Replace with proper error handling
    switch (message) {
    case DiagMessage::Unexpected:
        std::println(stderr, "lbc: error: unexpected token");
        break;
    case DiagMessage::NotImplemented:
        std::println(stderr, "lbc: error: not implemented");
        break;
    }
    std::exit(EXIT_FAILURE);
}

auto Parser::unexpected()const -> Error {
    (void)this;
    return Error(DiagMessage::Unexpected);
}

auto Parser::notImplemented()const -> Error {
    (void)this;
    return Error(DiagMessage::NotImplemented);
}

void Parser::advance() {
    m_token = m_lexer.next();
}

auto Parser::accept(const TokenKind kind) -> bool {
    if (m_token.kind() == kind) {
        advance();
        return true;
    }
    return false;
}

auto Parser::expect(const TokenKind kind)const -> Result<void> {
    if (m_token.kind() != kind) {
        return unexpected();
    }
    return {};
}

auto Parser::consume(const TokenKind kind) -> Result<void> {
    TRY(expect(kind))
    advance();
    return {};
}
