//
// Created by Albert Varaksin on 12/02/2026.
//
#include "Parser.hpp"
#include "Ast/Ast.hpp"
#include "Driver/Context.hpp"
using namespace lbc;

Parser::Parser(Context& context, const unsigned id)
: m_lexer(context, id)
, m_lastLoc(m_lexer.range().Start) {
}

Parser::~Parser() = default;

// module = stmtList EOF .
auto Parser::parse() -> Result<AstModule*> {
    TRY(advance())

    TRY_DECL(stmts, stmtList())
    TRY(consume(TokenKind::EndOfFile))

    // create the module
    return make<AstModule>(stmts->getRange(), stmts);
}

auto Parser::unexpected(const std::source_location& location) -> DiagError {
    if (m_deferredError.isValid()) {
        return DiagError(std::exchange(m_deferredError, {}));
    }
    return diag(diagnostics::unexpected(m_token), m_token.getRange().Start, llvm::ArrayRef(m_token.getRange()), location);
}

auto Parser::advance() -> Result<void> {
    if (m_deferredError.isValid()) {
        return DiagError(std::exchange(m_deferredError, {}));
    }
    if (auto res = m_lexer.next(); res) {
        m_lastLoc = m_token.getRange().End;
        m_token = res.value();
    } else {
        m_token = Token(TokenKind::Invalid, m_lexer.range());
        m_deferredError = res.error();
    }
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
        if (m_deferredError.isValid()) {
            return DiagError(std::exchange(m_deferredError, {}));
        }
        return expected(kind);
    }
    return {};
}

auto Parser::consume(const TokenKind kind) -> Result<void> {
    TRY(expect(kind))
    return advance();
}

auto Parser::identifier() -> Result<llvm::StringRef> {
    TRY(expect(TokenKind::Identifier))
    const auto id = std::get<llvm::StringRef>(m_token.getValue().getValue());
    TRY(advance())
    return id;
}
