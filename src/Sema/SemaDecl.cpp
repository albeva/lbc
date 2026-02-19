//
// Created by Albert Varaksin on 19/02/2026.
//
#include "SemanticAnalyser.hpp"
using namespace lbc;

auto SemanticAnalyser::accept(AstVarDecl& /*ast*/) -> Result {
    return notImplemented();
}

auto SemanticAnalyser::accept(AstFuncDecl& /*ast*/) -> Result {
    return notImplemented();
}

auto SemanticAnalyser::accept(AstFuncParamDecl& /*ast*/) -> Result {
    return notImplemented();
}
