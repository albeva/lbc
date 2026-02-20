//
// Created by Albert Varaksin on 19/02/2026.
//
#include "SemanticAnalyser.hpp"
using namespace lbc;

auto SemanticAnalyser::accept(AstStmtList& ast) -> Result {
    for (auto& decl: ast.getDecls()) {
        TRY(visit(*decl));
        (void)decl;
    }
    return {};
}

auto SemanticAnalyser::accept(AstExprStmt& /*ast*/) -> Result {
    return notImplemented();
}

auto SemanticAnalyser::accept(AstDeclareStmt& /*ast*/) -> Result {
    return notImplemented();
}

auto SemanticAnalyser::accept(AstFuncStmt& /*ast*/) -> Result {
    return notImplemented();
}

auto SemanticAnalyser::accept(AstReturnStmt& /*ast*/) -> Result {
    return notImplemented();
}

auto SemanticAnalyser::accept(AstDimStmt& /*ast*/) -> Result {
    return notImplemented();
}

auto SemanticAnalyser::accept(AstAssignStmt& /*ast*/) -> Result {
    return notImplemented();
}

auto SemanticAnalyser::accept(AstIfStmt& /*ast*/) -> Result {
    return notImplemented();
}
