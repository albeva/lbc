//
// Created by Albert Varaksin on 08/03/2026.
//
#include "IR/lib/Function.hpp"
#include "IR/lib/Variable.hpp"
#include "IrGenerator.hpp"
#include "Symbol/Symbol.hpp"
using namespace lbc::ir::gen;

auto IrGenerator::accept(const AstVarDecl& ast) -> Result {
    auto* symbol = ast.getSymbol();
    auto* var = getContext().create<lib::Variable>(symbol);
    symbol->setOperand(var);

    // Emit variable declaration
    emit(makeVar(var, ast.getType()));

    // Emit initialiser if present
    if (auto* expr = ast.getExpr()) {
        TRY(visit(*expr));
        emit(makeStore(var, expr->getOperand()));
    }

    return {};
}

auto IrGenerator::accept(const AstFuncDecl& ast) const -> Result {
    auto* symbol = ast.getSymbol();
    auto* func = getContext().create<lib::Function>(getContext(), symbol);
    symbol->setOperand(func);
    return {};
}

auto IrGenerator::accept(const AstFuncParamDecl& ast) const -> Result {
    auto* symbol = ast.getSymbol();
    auto* var = getContext().create<lib::Variable>(symbol);
    symbol->setOperand(var);
    return {};
}
