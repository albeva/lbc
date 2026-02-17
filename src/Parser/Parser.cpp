//
// Created by Albert Varaksin on 12/02/2026.
//
#include "Parser.hpp"
#include "Ast/Ast.hpp"
#include "Driver/Context.hpp"
using namespace lbc;

Parser::Parser(Context& context, unsigned id)
: m_context(context)
, m_lexer(context, id) {
}

Parser::~Parser() = default;

auto Parser::parse() -> Result<AstModule*> {
    TRY_ASSIGN(m_token, m_lexer.next())

    while (true) {
        std::println("'{}'", m_token);
        TRY_ASSIGN(m_token, m_lexer.next())
        if (m_token.kind().isOneOf(TokenKind::EndOfFile, TokenKind::Invalid)) {
            break;
        }
    }
    return nullptr;
}

auto Parser::unexpected(std::source_location location) -> DiagError {
    return diag(Diagnostics::unexpectedToken(m_token), m_token.getRange().Start, llvm::ArrayRef(m_token.getRange()), location);
}

auto Parser::notImplemented(std::source_location location) -> DiagError {
    return diag(Diagnostics::notImplemented(), {}, {}, location);
}

auto Parser::advance() -> Result<void> {
    TRY_ASSIGN(m_token, m_lexer.next())
    return {};
}

auto Parser::accept(const TokenKind kind) -> Result<bool> {
    if (m_token.kind() == kind) {
        TRY(advance());
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
    return advance();
}
