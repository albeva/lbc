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

Result<const TypeRoot*> TypePass::visit(AstTypeExpr& ast) const {
    const auto visitor = Visitor{
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

    for (int i = 0; i < ast.dereference; i++) {
        type = type->getPointer(m_sem.getContext());
    }

    ast.type = type;
    return type;
}

Result<const TypeRoot*> TypePass::visit(AstIdentExpr& ast) const {
    auto* symbol = m_sem.getSymbolTable()->find(ast.name);
    if (symbol == nullptr) {
        llvm::errs() << "Undefined type "_t + ast.name << '\n';
        return ResultError{};
    }

    if (symbol->valueFlags().kind != ValueFlags::Kind::type) {
        llvm::errs() << symbol->name() << " is not a type" << '\n';
        return ResultError{};
    }

    if (symbol->getType() == nullptr) {
        TRY(m_sem.getDeclPass().define(*symbol->getDecl()));
    }

    ast.type = symbol->getType();
    return ast.type;
}

Result<const TypeRoot*> TypePass::visit(AstFuncDecl& ast) const {
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
        if (retType->isUDT()) {
            llvm::errs() << "Returning types by value is not implemented" << '\n';
            return ResultError{};
        }
    } else {
        retType = TypeVoid::get();
    }

    // function
    return TypeFunction::get(m_sem.getContext(), retType, std::move(paramTypes), ast.variadic);
}

Result<const TypeRoot*> TypePass::visit(AstTypeOf& ast) const {
    if (ast.type != nullptr) {
        return ast.type;
    }
    TRY(m_sem.visit(ast));
    return ast.type;
}
