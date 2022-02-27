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
#include "Type/TypeProxy.hpp"
#include "Driver/Context.hpp"

using namespace lbc;
using namespace Sem;

const TypeRoot* TypePass::visit(AstTypeExpr& ast) const noexcept {
    const auto visitor = Visitor{
        [](TokenKind kind) -> const TypeRoot* {
            return TypeRoot::fromTokenKind(kind);
        },
        [&](AstIdentExpr* ident) -> const TypeRoot* {
            return visit(*ident);
        },
        [&](AstFuncDecl* decl) -> const TypeRoot* {
            return visit(*decl);
        }
    };
    const TypeRoot* type = std::visit(visitor, ast.expr);

    for (auto deref = 0; deref < ast.dereference; deref++) {
        type = TypePointer::get(m_sem.getContext(), type);
    }
    ast.typeProxy = type->getProxy();
    return ast.typeProxy->getType();
}

const TypeRoot* TypePass::visit(AstIdentExpr& ast) const noexcept {
    auto* sym = m_sem.getSymbolTable()->find(ast.name);
    if (sym == nullptr) {
        fatalError("Undefined type "_t + ast.name);
    }

    if (sym->getFlags().type) {
        ast.typeProxy = sym->getTypeProxy();
        return ast.typeProxy->getType();
    }

    fatalError(""_t + sym->name() + " is not a type");
}

const TypeRoot* TypePass::visit(AstFuncDecl& ast) const noexcept {
    // parameters
    std::vector<const TypeRoot*> paramTypes;
    if (ast.params != nullptr) {
        paramTypes.reserve(ast.params->params.size());
        for (auto& param : ast.params->params) {
            paramTypes.emplace_back(visit(*param->typeExpr));
        }
    }

    // return type
    const TypeRoot* retType = nullptr;
    if (ast.retTypeExpr != nullptr) {
        retType = visit(*ast.retTypeExpr);
        if (retType->isUDT()) {
            fatalError("Returning types by value is not implemented");
        }
    } else {
        retType = TypeVoid::get();
    }

    // function
    return TypeFunction::get(m_sem.getContext(), retType, std::move(paramTypes), ast.variadic);
}
