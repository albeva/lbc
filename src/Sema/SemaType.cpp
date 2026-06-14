//
// Created by Albert Varaksin on 19/02/2026.
//
#include "SemanticAnalyser.hpp"
using namespace lbc;

auto SemanticAnalyser::accept(AstBuiltInType& ast) const -> Result {
    const auto kind = ast.getTokenKind();
    const auto* type = getTypeFactory().getType(kind);
    ast.setType(type);
    return {};
}

auto SemanticAnalyser::accept(AstPointerType& ast) -> Result {
    auto& base = *ast.getTypeExpr();
    TRY(visit(base));
    if (base.getType()->isReference()) {
        return diag(diagnostics::pointerToReference(), ast.getRange());
    }
    const Type* type = getTypeFactory().getPointer(base.getType());
    ast.setType(type);
    return {};
}

auto SemanticAnalyser::accept(AstReferenceType& ast) -> Result {
    auto& base = *ast.getTypeExpr();
    TRY(visit(base));
    if (base.getType()->isReference()) {
        return diag(diagnostics::referenceToReference(), ast.getRange());
    }
    const Type* type = getTypeFactory().getReference(base.getType());
    ast.setType(type);
    return {};
}

auto SemanticAnalyser::accept(AstConstType& ast) -> Result {
    auto& base = *ast.getTypeExpr();
    TRY(visit(base));
    const auto* baseType = base.getType();
    if (baseType->isReference()) {
        return diag(diagnostics::constToReference(), ast.getRange());
    }
    // Redundant CONST collapses (e.g. `INTEGER CONST CONST`, `CONST T CONST`).
    const Type* type = baseType->isConst() ? baseType : getTypeFactory().getConst(baseType);
    ast.setType(type);
    return {};
}
