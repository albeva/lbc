//
// Created by Albert Varaksin on 28/11/2024.
//
#pragma once
#include "pch.hpp"
#include "Ast/AstVisitor.hpp"
#include "Type/Type.hpp"

namespace lbc {
class Context;

// namespace VM {
//     // clang-format off
//     using Value = std::variant<
//         #define TYPE(ID, STR, KIND, CPP, ...) CPP,
//         ALL_TYPES(TYPE)
//         #undef TYPE
//         TokenValue::NullType
//     >;
//     // clang-format on
// } // namespace VM

class ConstantFolder final : AstExprVisitor<ConstantFolder, Result<TokenValue>> {
    friend AstExprVisitor;

public:
    explicit ConstantFolder(Context& context)
    : m_context(context) { }
    auto fold(AstExpr& ast) -> Result<void>;

private:
    AST_EXPR_VISITOR_DECLARE_CONTENT_FUNCS()

    static auto expression(const AstExpr& ast) -> Result<TokenValue>;
    [[nodiscard]] auto stringBinaryExpr(TokenKind op, const TokenValue::StringType& lhs, const TokenValue::StringType& rhs) const -> TokenValue;
    static auto booleanBinaryExpr(TokenKind op, bool lhs, bool rhs) -> TokenValue;
    static auto booleanUnaryOperation(TokenKind op, bool operand) -> TokenValue;
    Context& m_context;
};

} // namespace lbc
