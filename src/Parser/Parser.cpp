//
// Created by Albert Varaksin on 12/02/2026.
//
#include "Parser.hpp"
#include "Ast/Ast.hpp"
#include "Driver/Context.hpp"
using namespace lbc;

Parser::Parser(Context& context, unsigned id)
: m_context(context)
, m_lexer(context, id)
, m_token(m_lexer.next()) { }

Parser::~Parser() = default;

auto Parser::parse() -> Result<AstModule*> {
    return notImplemented();
    // while (true) {
    //     std::println("'{}'", m_token);
    //     m_token = m_lexer.next();
    //     if (m_token.kind().isOneOf(TokenKind::EndOfFile, TokenKind::Invalid)) {
    //         break;
    //     }
    // }
    // return nullptr;
}

auto Parser::unexpected(std::source_location location) -> DiagError {
    return diag(Diagnostics::unexpectedToken(m_token), m_token.getRange().Start, {}, location);
}

auto Parser::notImplemented(std::source_location location) -> DiagError {
    return diag(Diagnostics::notImplemented(), {}, {}, location);
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

auto Parser::expect(const TokenKind kind) -> Result<void> {
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
