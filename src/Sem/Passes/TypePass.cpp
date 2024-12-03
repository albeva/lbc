//
// Created by Albert Varaksin on 22/05/2021.
//
#include "TypePass.hpp"
#include "Ast/Ast.hpp"
#include "Driver/Context.hpp"
#include "Sem/SemanticAnalyzer.hpp"
#include "Symbol/Symbol.hpp"
#include "Type/Type.hpp"

using namespace lbc;
using namespace Sem;

auto TypePass::visit(AstTypeExpr& ast) const -> Result<const TypeRoot*> {
    const auto visitor = Visitor {
        [&](AstIdentExpr* ident) -> Result<const TypeRoot*> {
            return visit(*ident);
        },
        [&](AstFuncDecl* decl) -> Result<const TypeRoot*> {
            return visit(*decl);
        },
        [&](AstTypeOf* typeOf) -> Result<const TypeRoot*> {
            return visit(*typeOf);
        },
        [](TokenKind kind) -> Result<const TypeRoot*> {
            return TypeRoot::fromTokenKind(kind);
        }
    };
    TRY_DECL(type, std::visit(visitor, ast.expr))

    for (int i = 0; i < ast.indirection; i++) {
        type = type->getPointer(m_sem.getContext());
    }

    if (ast.isReference) {
        type = type->getReference(m_sem.getContext());
    }

    ast.type = type;
    return type;
}

auto TypePass::visit(AstIdentExpr& ast) const -> Result<const TypeRoot*> {
    auto* symbol = m_sem.getSymbolTable()->find(ast.name);
    if (symbol == nullptr) {
        return m_sem.makeError(Diag::undefinedType, ast, ast.name);
    }

    if (symbol->valueFlags().kind != ValueFlags::Kind::Type) {
        return m_sem.makeError(Diag::notAType, ast, ast.name);
    }

    if (symbol->getType() == nullptr) {
        TRY(m_sem.getDeclPass().define(*symbol->getDecl()))
    }

    ast.type = symbol->getType();
    return ast.type;
}

auto TypePass::visit(AstFuncDecl& ast) const -> Result<const TypeRoot*> {
    // parameters
    llvm::SmallVector<const TypeRoot*> paramTypes;
    if (ast.params != nullptr) {
        paramTypes.reserve(ast.params->params.size());
        for (const auto& param : ast.params->params) {
            TRY_DECL(type, visit(*param->typeExpr))
            paramTypes.emplace_back(type);
        }
    }

    // return type
    const TypeRoot* retType = nullptr;
    if (ast.retTypeExpr != nullptr) {
        TRY_ASSIGN(retType, visit(*ast.retTypeExpr))
    } else {
        retType = TypeVoid::get();
    }

    // function
    return TypeFunction::get(m_sem.getContext(), retType, std::move(paramTypes), ast.variadic);
}

auto TypePass::visit(AstTypeOf& ast) const -> Result<const TypeRoot*> {
    if (ast.type != nullptr) {
        return ast.type;
    }
    TRY(m_sem.visit(ast))
    return ast.type;
}
