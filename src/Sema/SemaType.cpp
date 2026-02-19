//
// Created by Albert Varaksin on 19/02/2026.
//
#include "SemanticAnalyser.hpp"
using namespace lbc;

auto SemanticAnalyser::accept(AstBuiltInType& /*ast*/) -> Result {
    return notImplemented();
}

auto SemanticAnalyser::accept(AstPointerType& /*ast*/) -> Result {
    return notImplemented();
}

auto SemanticAnalyser::accept(AstReferenceType& /*ast*/) -> Result {
    return notImplemented();
}
