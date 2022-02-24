//
// Created by Albert Varaksin on 01/05/2021.
//
#include "FuncDeclarerPass.hpp"
#include "Ast/Ast.hpp"
#include "Driver/Context.hpp"
#include "Sem/SemanticAnalyzer.hpp"
#include "Symbol/SymbolTable.hpp"
#include "Type/Type.hpp"
#include "TypePass.hpp"
using namespace lbc;
using namespace Sem;

void FuncDeclarerPass::visit(AstModule& ast) {
    m_sem.with(ast.symbolTable, [&]() {
        visit(*ast.stmtList);
    });
}

void FuncDeclarerPass::visit(AstStmtList& ast) {
    for (const auto& stmt : ast.stmts) {
        switch (stmt->kind) {
        case AstKind::FuncDecl:
            visitFuncDecl(static_cast<AstFuncDecl&>(*stmt), true);
            break;
        case AstKind::FuncStmt:
            visitFuncDecl(*static_cast<AstFuncStmt&>(*stmt).decl, false);
            break;
        case AstKind::Import: {
            auto& import = static_cast<AstImport&>(*stmt);
            if (import.module != nullptr) {
                visit(*import.module->stmtList);
            }
            break;
        }
        default:
            break;
        }
    }
}

void FuncDeclarerPass::visitFuncDecl(AstFuncDecl& ast, bool external) {
    auto* symbolTale = m_sem.getSymbolTable();

    const auto& name = ast.name;
    if (symbolTale->exists(name)) {
        fatalError("Redefinition of "_t + name);
    }
    auto* symbol = symbolTale->insert(m_sem.getContext(), name);
    auto flags = symbol->getFlags();
    flags.callable = true;
    flags.addressable = true;
    symbol->setFlags(flags);

    // alias?
    if (ast.attributes != nullptr) {
        if (auto alias = ast.attributes->getStringLiteral("ALIAS")) {
            symbol->setAlias(*alias);
        }
    }

    if (symbol->name() == "MAIN" && symbol->alias().empty()) {
        symbol->setAlias("main");
        symbol->setExternal(true);
    } else {
        symbol->setExternal(external);
    }

    // parameters
    std::vector<const TypeRoot*> paramTypes;
    ast.symbolTable = m_sem.getContext().create<SymbolTable>(symbolTale);
    if (ast.params != nullptr) {
        paramTypes.reserve(ast.params->params.size());
        m_sem.with(ast.symbolTable, [&]() {
            for (auto& param : ast.params->params) {
                visitFuncParamDecl(*param);
                paramTypes.emplace_back(param->symbol->type());
            }
        });
    }

    // return typeExpr. subs don't have one so default to Void
    const TypeRoot* retType = nullptr;
    if (ast.retTypeExpr != nullptr) {
        m_sem.getTypePass().visit(*ast.retTypeExpr);
        retType = ast.retTypeExpr->type;
        if (retType->isUDT()) {
            fatalError("Returning types by value is not implemented");
        }
    } else {
        retType = TypeVoid::get();
    }

    // create function symbol
    const auto* type = TypeFunction::get(m_sem.getContext(), retType, std::move(paramTypes), ast.variadic);
    ast.type = type;
    symbol->setType(type);
    ast.symbol = symbol;
}

void FuncDeclarerPass::visitFuncParamDecl(AstFuncParamDecl& ast) {
    auto* symbol = createParamSymbol(ast);

    m_sem.getTypePass().visit(*ast.typeExpr);
    symbol->setType(ast.typeExpr->type);

    if (symbol->type()->isUDT()) {
        fatalError("Passing types by values is not implemented");
    }

    ast.symbol = symbol;
    ast.type = symbol->type();
}

Symbol* FuncDeclarerPass::createParamSymbol(AstFuncParamDecl& ast) {
    const auto& name = ast.name;
    if (m_sem.getSymbolTable()->find(name, false) != nullptr) {
        fatalError("Redefinition of "_t + name);
    }
    auto* symbol = m_sem.getSymbolTable()->insert(m_sem.getContext(), name);

    // alias?
    if (ast.attributes != nullptr) {
        if (auto alias = ast.attributes->getStringLiteral("ALIAS")) {
            symbol->setAlias(*alias);
        }
    }

    return symbol;
}