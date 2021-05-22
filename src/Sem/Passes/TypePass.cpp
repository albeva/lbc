//
// Created by Albert on 22/05/2021.
//
#include "TypePass.h"
#include "Ast/Ast.h"
#include "Type/Type.h"

using namespace lbc;
using namespace Sem;

void TypePass::visit(AstTypeExpr* ast) noexcept {
    const auto* type = TypeRoot::fromTokenKind(ast->tokenKind);
    for (auto deref = 0; deref < ast->dereference; deref++) {
        type = TypePointer::get(type);
    }
    ast->type = type;
}