//
// Created by Albert Varaksin on 22/05/2021.
//
#include "TypePass.hpp"
#include "Ast/Ast.hpp"
#include "Driver/Context.hpp"
#include "Sem/SemanticAnalyzer.hpp"
#include "Symbol/Symbol.hpp"
#include "Symbol/SymbolTable.hpp"
#include "Type/Type.hpp"
#include "Type/TypeProxy.hpp"
#include "Type/TypeUdt.hpp"

using namespace lbc;
using namespace Sem;

TypeProxy* TypePass::visit(AstTypeExpr& ast) const noexcept {
    const auto visitor = Visitor{
        [](TokenKind kind) -> TypeProxy* {
            return TypeRoot::fromTokenKind(kind)->getProxy();
        },
        [&](AstIdentExpr* ident) -> TypeProxy* {
            return visit(*ident);
        },
        [&](AstFuncDecl* decl) -> TypeProxy* {
            return visit(*decl);
        }
    };
    auto* proxy = std::visit(visitor, ast.expr);

    for (auto deref = 0; deref < ast.dereference; deref++) {
        proxy = TypePointer::get(m_sem.getContext(), proxy->getType())->getProxy();
    }
    ast.typeProxy = proxy;
    return proxy;
}

TypeProxy* TypePass::visit(AstIdentExpr& ast) const noexcept {
    auto* sym = m_sem.getSymbolTable()->find(ast.name);
    if (sym == nullptr) {
        fatalError("Undefined type "_t + ast.name);
    }

    if (sym->getFlags().type) {
        ast.typeProxy = sym->getTypeProxy();
        return ast.typeProxy;
    }

    fatalError(""_t + sym->name() + " is not a type");
}

TypeProxy* TypePass::visit(AstFuncDecl& ast) const noexcept {
    // parameters
    std::vector<const TypeRoot*> paramTypes;
    if (ast.params != nullptr) {
        paramTypes.reserve(ast.params->params.size());
        for (auto& param : ast.params->params) {
            paramTypes.emplace_back(visit(*param->typeExpr)->getType());
        }
    }

    // return type
    const TypeProxy* retType = nullptr;
    if (ast.retTypeExpr != nullptr) {
        retType = visit(*ast.retTypeExpr);
        if (retType->getType()->isUDT()) {
            fatalError("Returning types by value is not implemented");
        }
    } else {
        retType = TypeVoid::get()->getProxy();
    }

    // function
    return TypeFunction::get(m_sem.getContext(), retType->getType(), std::move(paramTypes), ast.variadic)->getProxy();
}
