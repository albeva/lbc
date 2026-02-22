//
// Created by Albert Varaksin on 19/02/2026.
//
#include "Driver/Context.hpp"
#include "SemanticAnalyser.hpp"
#include "Symbol/Symbol.hpp"
#include "Symbol/SymbolTable.hpp"
#include "Type/TypeFactory.hpp"
using namespace lbc;

auto SemanticAnalyser::declare(AstDecl& ast) -> Result {
    if (m_symbolTable->contains(ast.getName(), false)) {
        return diag(diagnostics::redefinition(ast.getName()), ast.getRange().Start, ast.getRange());
    }

    auto* symbol = m_context.create<Symbol>(ast.getName(), ast.getType(), ast.getRange());
    m_symbolTable->insert(symbol);
    symbol->setVisibility(SymbolVisibility::Private);

    if (llvm::isa<AstFuncDecl>(&ast)) {
        symbol->setFlag(SymbolFlags::Function);
    } else if (llvm::isa<AstVarDecl>(&ast)) {
        symbol->setFlag(SymbolFlags::Variable);
    }

    ast.setSymbol(symbol);
    return {};
}

auto SemanticAnalyser::define(AstDecl& ast) -> Result {
    auto* symbol = ast.getSymbol();
    if (symbol->hasFlag(SymbolFlags::Defined)) {
        return {};
    }

    if (symbol->hasFlag(SymbolFlags::BeingDefined)) {
        return diag(diagnostics::circularDependency(symbol->getName()), {}, symbol->getRange());
    }

    symbol->setFlag(SymbolFlags::BeingDefined);
    TRY(visit(ast));
    symbol->unsetFlag(SymbolFlags::BeingDefined);
    symbol->setFlag(SymbolFlags::Defined);

    return {};
}

auto SemanticAnalyser::accept(AstVarDecl& ast) -> Result {
    const Type* type = nullptr;
    if (auto* ty = ast.getTypeExpr()) {
        TRY(visit(*ty));
        type = ty->getType();
    }
    if (auto* expr = ast.getExpr()) {
        expr->setType(type);
        TRY(visit(*expr));
        type = expr->getType();
    }
    ast.setType(type);
    ast.getSymbol()->setType(type);
    return {};
}

auto SemanticAnalyser::accept(AstFuncDecl& ast) -> Result {
    const auto count = ast.getParams().size();

    auto related = m_context.span<Symbol*>(count);
    auto params = m_context.span<const Type*>(count);

    // Process parameters
    std::size_t index = 0;
    for (auto* param : ast.getParams()) {
        TRY(visit(*param));
        related[index] = param->getSymbol();
        params[index] = param->getType();
        index++;
    }

    // Process return type
    const Type* returnType = nullptr;
    if (auto* retTyExpr = ast.getRetTypeExpr()) {
        TRY(visit(*retTyExpr));
        returnType = retTyExpr->getType();
    } else {
        returnType = m_context.getTypeFactory().getVoid();
    }

    // Get function type
    const auto* funcType = getTypeFactory().getFunction(params, returnType);
    ast.setType(funcType);

    // create symbol
    Symbol* symbol = ast.getSymbol();
    symbol->setType(funcType);
    symbol->setRelatedSymbols(related);

    // done
    return {};
}

auto SemanticAnalyser::accept(AstFuncParamDecl& ast) -> Result {
    auto& typeExpr = *ast.getTypeExpr();

    // the type
    TRY(visit(typeExpr));
    ast.setType(typeExpr.getType());

    // the symbol
    TRY(declare(ast))
    ast.getSymbol()->setFlag(SymbolFlags::Defined);

    return {};
}
