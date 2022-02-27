//
// Created by Albert on 29/05/2021.
//
#include "UdtDeclPass.hpp"
#include "Ast/Ast.hpp"
#include "Driver/Context.hpp"
#include "Sem/SemanticAnalyzer.hpp"
#include "Type/Type.hpp"
#include "Type/TypeProxy.hpp"
#include "Type/TypeUdt.hpp"
using namespace lbc;
using namespace Sem;

void UdtDeclPass::visit(AstModule& ast) noexcept {
    m_sem.with(ast.symbolTable, [&]() {
        visit(*ast.stmtList);
    });
}

void UdtDeclPass::visit(AstStmtList& ast) noexcept {
    for (auto& stmt : ast.stmts) {
        switch (stmt->kind) {
        case AstKind::UdtDecl:
            visit(static_cast<AstUdtDecl&>(*stmt));
            break;
        default:
            break;
        }
    }
}

void UdtDeclPass::visit(AstUdtDecl& ast) noexcept {
    auto* symbol = m_sem.createNewSymbol(ast);
    symbol->setTypeProxy(m_sem.getContext().create<TypeProxy>());

    bool packed = false;
    if (ast.attributes != nullptr) {
        packed = ast.attributes->exists("PACKED");
    }
    ast.symbolTable = m_sem.getContext().create<SymbolTable>(m_sem.getSymbolTable());

    m_sem.with(ast.symbolTable, [&]() {
        for (const auto& decl : ast.decls->decls) {
            m_sem.visit(*decl);
            decl->symbol->setParent(symbol);
        }
    });

    ast.symbolTable->setParent(nullptr);
    TypeUDT::get(m_sem.getContext(), *symbol, *ast.symbolTable, packed);
}
