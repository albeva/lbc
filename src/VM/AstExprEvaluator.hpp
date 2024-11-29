//
// Created by Albert Varaksin on 28/11/2024.
//
#pragma once
#include "pch.hpp"
#include "Ast/AstVisitor.hpp"
#include "Type/Type.hpp"
#include "VariableStack.hpp"

namespace lbc {
class Context;
class SymbolTable;

class AstExprEvaluator final : public AstExprVisitor<AstExprEvaluator, Result<void>> {
    NO_COPY_AND_MOVE(AstExprEvaluator)
public:
    AstExprEvaluator();
    ~AstExprEvaluator();

    auto evaluate(AstExpr& ast) -> Result<void>;

    AST_EXPR_VISITOR_DECLARE_CONTENT_FUNCS()
private:
    auto expr(AstExpr& ast) -> Result<void>;

    VariableStack m_stack;
};

} // namespace lbc
