//
// Created by Albert Varaksin on 08/03/2026.
//
#include "IrGenerator.hpp"
using namespace lbc::ir::gen;

auto IrGenerator::accept(const AstVarDecl& /*ast*/) -> Result {
    return notImplemented();
}

auto IrGenerator::accept(const AstFuncDecl& /*ast*/) -> Result {
    return notImplemented();
}

auto IrGenerator::accept(const AstFuncParamDecl& /*ast*/) -> Result {
    return notImplemented();
}
