//
// Created by Albert Varaksin on 12/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Cursor.hpp"
#include "Token.hpp"
namespace lbc {
class Context;

class Lexer final {
public:
    NO_COPY_AND_MOVE(Lexer)
    Lexer(Context& context, unsigned id);

    /**
     * Get next token from the input.
     */
    [[nodiscard]] auto next() -> Token;

private:
    [[maybe_unused]] Context& m_context;
    [[maybe_unused]] unsigned m_id;
    [[maybe_unused]] Cursor m_current {}, m_end {};
};

} // namespace lbc
