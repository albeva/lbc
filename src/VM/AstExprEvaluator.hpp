//
// Created by Albert Varaksin on 28/11/2024.
//
#pragma once
#include "pch.hpp"
#include "Ast/AstVisitor.hpp"
#include "Type/Type.hpp"

namespace lbc {
class Context;
class SymbolTable;

class AstExprEvaluator final: public AstExprVisitor<AstExprEvaluator, Result<void>> {
    NO_COPY_AND_MOVE(AstExprEvaluator)
public:
    AstExprEvaluator(Context& context, SymbolTable& symbolTable);
    ~AstExprEvaluator();

    auto evaluate(AstExpr& expr) -> Result<void>;

    AST_EXPR_VISITOR_DECLARE_CONTENT_FUNCS()

    using Value = std::variant<
        #define TYPE(ID, STR, KIND, CPP, ...) CPP,
        ALL_TYPES(TYPE)
        #undef TYPE
        std::monostate
    >;
    using ValueStack = std::stack<Value, std::vector<Value>>;

private:

    ValueStack m_stack;
    Context& m_context;
    SymbolTable& m_symbolTable;
};

} // namespace lbc
