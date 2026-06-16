//
// Created by Albert Varaksin on 19/02/2026.
//
#include "SemanticAnalyser.hpp"
#include "Symbol/Symbol.hpp"
#include "Type/Aggregate.hpp"
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
    for (const auto& decl : ast.getDecls()) {
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

auto SemanticAnalyser::accept(AstFuncStmt& ast) -> Result {
    const auto* funcType = llvm::cast<TypeFunction>(ast.getDecl()->getType());

    // Track the active return type so RETURN statements within the body can be
    // checked. The body's scope — already populated with the parameters by the
    // function declaration — is analysed here.
    const ValueRestorer restore { m_returnType };
    m_returnType = funcType->getReturnType();

    return accept(*ast.getStmtList());
}

auto SemanticAnalyser::accept(AstReturnStmt& ast) -> Result {
    // RETURN is only valid inside a function or subroutine body.
    if (m_returnType == nullptr) {
        return diag(diagnostics::returnOutsideFunction(), ast.getRange());
    }

    if (ast.getExpr() != nullptr) {
        // A subroutine has no value to return.
        if (m_returnType->isVoid()) {
            return diag(diagnostics::returnValueInSub(), ast.getRange());
        }
        TRY_EXPRESSION(ast, Expr, m_returnType)
    } else if (not m_returnType->isVoid()) {
        // A function must return a value.
        return diag(diagnostics::returnMissingValue(), ast.getRange());
    }

    return {};
}

auto SemanticAnalyser::accept(const AstDimStmt& ast) -> Result {
    for (auto* decl : ast.getDecls()) {
        TRY(define(*decl));
    }
    return {};
}

auto SemanticAnalyser::accept(AstAssignStmt& ast) -> Result {
    TRY(visit(*ast.getAssignee()))
    TRY(ensureAssignable(*ast.getAssignee()))
    TRY_EXPRESSION(ast, Expr, ast.getAssignee()->getType())
    return {};
}

auto SemanticAnalyser::accept(AstIfStmt& /*ast*/) -> Result {
    return notImplemented();
}

auto SemanticAnalyser::accept(const AstExtern& ast) -> Result {
    // Declare the block's declarations under its linkage — this is where each
    // symbol's verbatim alias is set. Definitions then run with the default
    // linkage, so nested parameters are not themselves aliased.
    {
        const ValueRestorer restore { m_externKind };
        m_externKind = ast.getExternKind();
        for (auto* stmt : ast.getStmts()) {
            TRY(declare(*llvm::cast<AstDeclareStmt>(stmt)->getDecl()))
        }
    }
    for (auto* stmt : ast.getStmts()) {
        TRY(define(*llvm::cast<AstDeclareStmt>(stmt)->getDecl()))
    }
    return {};
}
