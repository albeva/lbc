//
// Created by Albert Varaksin on 19/02/2026.
//
#include "SemanticAnalyser.hpp"
using namespace lbc;

auto SemanticAnalyser::accept(AstCastExpr& /* ast */) -> Result {
    return notImplemented();
}

auto SemanticAnalyser::accept(AstVarExpr& /*ast*/) -> Result {
    return notImplemented();
}

auto SemanticAnalyser::accept(AstCallExpr& /*ast*/) -> Result {
    return notImplemented();
}

auto SemanticAnalyser::accept(AstLiteralExpr& /*ast*/) -> Result {
    return notImplemented();
}

auto SemanticAnalyser::accept(AstUnaryExpr& /*ast*/) -> Result {
    return notImplemented();
}

auto SemanticAnalyser::accept(AstBinaryExpr& /*ast*/) -> Result {
    return notImplemented();
}

auto SemanticAnalyser::accept(AstMemberExpr& /*ast*/) -> Result {
    return notImplemented();
}
