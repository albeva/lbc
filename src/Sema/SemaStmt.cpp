//
// Created by Albert Varaksin on 19/02/2026.
//
#include "SemanticAnalyser.hpp"
#include "Symbol/Symbol.hpp"
using namespace lbc;

auto SemanticAnalyser::accept(AstStmtList& ast) -> Result {
    const ValueRestorer restore { m_symbolTable };

    // Set active symbol table (scope)
    if (auto* symbolTable = ast.getSymbolTable()) {
        m_symbolTable = symbolTable;
    } else {
        m_symbolTable = m_context.create<SymbolTable>(m_symbolTable);
        ast.setSymbolTable(m_symbolTable);
    }

    // declare symbols
    for (auto& decl : ast.getDecls()) {
        TRY(declare(*decl));
    }

    // define symbols
    for (auto& decl : ast.getDecls()) {
        if (llvm::isa<AstFuncDecl>(decl)) {
            TRY(define(*decl));
        }
    }

    // process statements
    for (auto& stmt : ast.getStmts()) {
        TRY(visit(*stmt));
    }

    // done
    return {};
}

auto SemanticAnalyser::accept(AstExprStmt& ast) -> Result {
    TRY_EXPRESSION(ast, Expr, nullptr)
    return {};
}

auto SemanticAnalyser::accept(AstDeclareStmt& /*ast*/) -> Result {
    /* NO OP, handled in stmtList */
    return {};
}

auto SemanticAnalyser::accept(AstFuncStmt& /*ast*/) -> Result {
    return notImplemented();
}

auto SemanticAnalyser::accept(AstReturnStmt& /*ast*/) -> Result {
    return notImplemented();
}

auto SemanticAnalyser::accept(AstDimStmt& ast) -> Result {
    for (auto* decl : ast.getDecls()) {
        TRY(define(*decl));
    }
    return {};
}

auto SemanticAnalyser::accept(AstAssignStmt& ast) -> Result {
    TRY(visit(*ast.getAssignee()))
    TRY_EXPRESSION(ast, Expr, ast.getAssignee()->getType())
    return {};
}

auto SemanticAnalyser::accept(AstIfStmt& /*ast*/) -> Result {
    return notImplemented();
}
