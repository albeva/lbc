//
// Created by Albert Varaksin on 28/11/2024.
//
#pragma once
#include "pch.hpp"
#include "Ast/AstVisitor.hpp"
#include "Type/Type.hpp"

namespace lbc {
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

class AstExprEvaluator final : public AstExprVisitor<AstExprEvaluator, Result<VM::Value>> {
    NO_COPY_AND_MOVE(AstExprEvaluator)
public:
    AstExprEvaluator();
    ~AstExprEvaluator();

    auto evaluate(AstExpr& ast) -> Result<void>;

    AST_EXPR_VISITOR_DECLARE_CONTENT_FUNCS()
};

} // namespace lbc
