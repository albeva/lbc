//
// Created by Albert Varaksin on 28/11/2024.
//
#pragma once
#include "pch.hpp"
#include "Ast/AstVisitor.hpp"
#include "Type/Type.hpp"

namespace lbc {
class Context;


namespace VM {
    // clang-format off
    using Value = std::variant<
        #define TYPE(ID, STR, KIND, CPP, ...) CPP,
        ALL_TYPES(TYPE)
        #undef TYPE
        TokenValue::NullType
    >;
    // clang-format on
} // namespace VM

struct AstExprEvaluator final : private AstExprVisitor<AstExprEvaluator, Result<VM::Value>> {
    explicit AstExprEvaluator(Context& context) : m_context(context) {}
    auto evaluate(AstExpr& ast) -> Result<void>;

private:
    friend AstExprVisitor;
    AST_EXPR_VISITOR_DECLARE_CONTENT_FUNCS()

    static auto expression(const AstExpr& ast) -> Result<VM::Value>;
    [[nodiscard]] auto stringBinaryExpr(TokenKind op, const TokenValue::StringType& lhs, const TokenValue::StringType& rhs) const -> VM::Value;
    static auto booleanBinaryExpr(TokenKind op, bool lhs, bool rhs) -> VM::Value;
    static auto booleanUnaryOperation(TokenKind op, bool operand) -> VM::Value;
    Context& m_context;
};

} // namespace lbc
