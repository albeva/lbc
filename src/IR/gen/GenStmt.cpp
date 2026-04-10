//
// Created by Albert Varaksin on 08/03/2026.
//
#include "IrGenerator.hpp"
using namespace lbc::ir::gen;

auto IrGenerator::accept(const AstStmtList& ast) -> Result {
    for (auto* decl : ast.getDecls()) {
        TRY(visit(*decl));
    }
    for (auto* stmt : ast.getStmts()) {
        TRY(visit(*stmt));
    }
    return {};
}

auto IrGenerator::accept(const AstExprStmt& /*ast*/) -> Result {
    return notImplemented();
}

auto IrGenerator::accept(const AstDeclareStmt& /*ast*/) -> Result {
    return notImplemented();
}

auto IrGenerator::accept(const AstFuncStmt& /*ast*/) -> Result {
    return notImplemented();
}

auto IrGenerator::accept(const AstReturnStmt& /*ast*/) -> Result {
    return notImplemented();
}

auto IrGenerator::accept(const AstDimStmt& /*ast*/) -> Result {
    return notImplemented();
}

auto IrGenerator::accept(const AstAssignStmt& /*ast*/) -> Result {
    return notImplemented();
}

auto IrGenerator::accept(const AstIfStmt& /*ast*/) -> Result {
    return notImplemented();
}
