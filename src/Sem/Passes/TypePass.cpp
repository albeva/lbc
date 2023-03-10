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
#include "Type/TypeUdt.hpp"

using namespace lbc;
using namespace Sem;

const TypeRoot* TypePass::visit(AstTypeExpr& ast) const {
    const auto visitor = Visitor{
        [&](AstIdentExpr* ident) -> const TypeRoot* {
            return visit(*ident);
        },
        [&](AstFuncDecl* decl) -> const TypeRoot* {
            return visit(*decl);
        },
        [&](AstTypeOf* typeOf) -> const TypeRoot* {
            return visit(*typeOf);
        },
        [](TokenKind kind) -> const TypeRoot* {
            return TypeRoot::fromTokenKind(kind);
        }
    };
    const auto* type = std::visit(visitor, ast.expr);

    for (int i = 0; i < ast.dereference; i++) {
        type = type->getPointer(m_sem.getContext());
    }

    ast.type = type;
    return type;
}

const TypeRoot* TypePass::visit(AstIdentExpr& ast) const {
    auto* symbol = m_sem.getSymbolTable()->find(ast.name);
    if (symbol == nullptr) {
        fatalError("Undefined type "_t + ast.name);
    }

    if (symbol->valueFlags().kind != ValueFlags::Kind::type) {
        fatalError(""_t + symbol->name() + " is not a type");
    }

    if (symbol->getType() == nullptr) {
        m_sem.getDeclPass().define(symbol);
    }

    ast.type = symbol->getType();
    return ast.type;
}

const TypeRoot* TypePass::visit(AstFuncDecl& ast) const {
    // parameters
    std::vector<const TypeRoot*> paramTypes;
    if (ast.params != nullptr) {
        paramTypes.reserve(ast.params->params.size());
        for (const auto& param : ast.params->params) {
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

const TypeRoot* TypePass::visit(AstTypeOf& ast) const {
    if (ast.type != nullptr) {
        return ast.type;
    }
    MUST(m_sem.visit(ast));
    return ast.type;
}
