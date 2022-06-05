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

TypeProxy* TypePass::visit(AstTypeExpr& ast, TypeProxy* owner) const noexcept {
    const auto visitor = Visitor{
        [&](AstIdentExpr* ident) -> TypeProxy* {
            return visit(*ident);
        },
        [&](AstFuncDecl* decl) -> TypeProxy* {
            return visit(*decl);
        },
        [&](AstTypeOf* typeOf) -> TypeProxy* {
            return visit(*typeOf);
        },
        [](TokenKind kind) -> TypeProxy* {
            return TypeRoot::fromTokenKind(kind)->getProxy();
        }
    };
    auto* proxy = std::visit(visitor, ast.expr);

    if (ast.dereference > 0) {
        if (const auto* type = proxy->getType()) {
            for (int i = 0; i < ast.dereference; i++) {
                type = type->getPointer(m_sem.getContext());
            }
            proxy = type->getProxy();
        } else if (owner == nullptr) {
            proxy = m_sem.getContext().create<TypeProxy>(proxy);
            proxy->setDereference(ast.dereference, &m_sem.getContext());
        } else {
            owner->setDereference(ast.dereference, &m_sem.getContext());
        }
    }

    ast.typeProxy = proxy;
    return proxy;
}

TypeProxy* TypePass::visit(AstIdentExpr& ast) const noexcept {
    auto* symbol = m_sem.getSymbolTable()->find(ast.name);
    if (symbol == nullptr) {
        fatalError("Undefined type "_t + ast.name);
    }

    if (!symbol->valueFlags().isType) {
        fatalError(""_t + symbol->name() + " is not a type");
    }

    if (not symbol->stateFlags().defined) {
        m_sem.getDeclPass().define(*symbol->getDecl());
    }

    ast.typeProxy = symbol->getTypeProxy();
    return ast.typeProxy;
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
    TypeProxy* retType = nullptr;
    if (ast.retTypeExpr != nullptr) {
        retType = visit(*ast.retTypeExpr);
        if (retType->getType()->isUDT()) {
            fatalError("Returning types by value is not implemented");
        }
    } else {
        retType = TypeVoid::get()->getProxy();
    }

    // function
    const auto* type = TypeFunction::get(m_sem.getContext(), retType->getType(), std::move(paramTypes), ast.variadic);
    return type->getProxy();
}

TypeProxy* TypePass::visit(AstTypeOf& ast) const noexcept {
    if (ast.typeProxy != nullptr) {
        return ast.typeProxy;
    }
    m_sem.visit(ast);
    return ast.typeProxy;
}
