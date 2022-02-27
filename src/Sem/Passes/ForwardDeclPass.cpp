//
// Created by Albert on 26/02/2022.
//
#include "ForwardDeclPass.hpp"
#include "Sem/SemanticAnalyzer.hpp"
#include "Ast/Ast.hpp"
#include "Type/TypeProxy.hpp"
#include "Driver/Context.hpp"
using namespace lbc;
using namespace Sem;

namespace {
    template<typename Callable>
    inline void iterate(AstStmtList& ast, Callable callable) noexcept {
        for (auto& stmt : ast.stmts) {
            if (auto* decl = llvm::dyn_cast<AstDecl>(stmt)) {
                callable(*decl);
            }
        }
    }

    template<typename Callable>
    inline void iterate(AstDeclList& ast, Callable callable) noexcept {
        for (auto& decl : ast.decls) {
            callable(*decl);
        }
    }
} // namespace

void ForwardDeclPass::visit(AstModule& ast) noexcept {
    iterate(*ast.stmtList, [&](auto& decl) { declare(decl); });
}

void ForwardDeclPass::declare(AstDecl& ast) noexcept {
    if (not llvm::isa<AstFuncDecl, AstUdtDecl, AstTypeAlias>(ast)) {
        return;
    }

    auto* symbol = m_sem.createNewSymbol(ast);
    symbol->setDecl(&ast);
    symbol->setTypeProxy(m_sem.getContext().create<TypeProxy>());
    ast.symbol = symbol;

    if (llvm::isa<AstFuncDecl>(ast)) {
        m_types.push_back(symbol);
    } else {
        m_procs.push_back(symbol);
    }
}
