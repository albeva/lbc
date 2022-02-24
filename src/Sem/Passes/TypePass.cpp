//
// Created by Albert Varaksin on 22/05/2021.
//
#include "TypePass.hpp"
#include "Ast/Ast.hpp"
#include "Sem/SemanticAnalyzer.hpp"
#include "Symbol/Symbol.hpp"
#include "Symbol/SymbolTable.hpp"
#include "Type/Type.hpp"
#include "Type/TypeUdt.hpp"

using namespace lbc;
using namespace Sem;

void TypePass::visit(AstTypeExpr& ast) const noexcept {
    const auto visitor = Visitor{
        [](TokenKind kind) -> const TypeRoot* {
            return TypeRoot::fromTokenKind(kind);
        },
        [&](AstIdentExpr* ident) -> const TypeRoot*  {
            visit(*ident);
            return ident->type;
        },
        [&](AstFuncDecl* decl) -> const TypeRoot* {
            visit(*decl);
            return decl->type;
        }
    };
    const TypeRoot* type = std::visit(visitor, ast.expr);

    for (auto deref = 0; deref < ast.dereference; deref++) {
        type = TypePointer::get(m_sem.getContext(), type);
    }

    ast.type = type;
}

void TypePass::visit(AstIdentExpr& ast) const noexcept {
    auto* sym = m_sem.getSymbolTable()->find(ast.name);
    if (sym == nullptr) {
        fatalError("Undefined type "_t + ast.name);
    }

    if (const auto* udt = dyn_cast<TypeUDT>(sym->type())) {
        ast.type = udt;
        return;
    }

    fatalError(""_t + sym->name() + " is not a type");
}

void TypePass::visit(AstFuncDecl& ast) const noexcept {
    // parameters
    std::vector<const TypeRoot*> paramTypes;
    if (ast.params != nullptr) {
        paramTypes.reserve(ast.params->params.size());
        for (auto& param : ast.params->params) {
            visit(*param->typeExpr);
            paramTypes.emplace_back(param->typeExpr->type);
        }
    }

    // return type
    const TypeRoot* retType = nullptr;
    if (ast.retTypeExpr != nullptr) {
        visit(*ast.retTypeExpr);
        retType = ast.retTypeExpr->type;
        if (retType->isUDT()) {
            fatalError("Returning types by value is not implemented");
        }
    } else {
        retType = TypeVoid::get();
    }

    // function
    ast.type = TypeFunction::get(m_sem.getContext(), retType, std::move(paramTypes), ast.variadic);
}
