//
// Created by Albert Varaksin on 08/03/2026.
//
#include "IrGenerator.hpp"
using namespace lbc::ir::gen;

auto IrGenerator::accept(const AstBuiltInType& /*ast*/) -> Result {
    return notImplemented();
}

auto IrGenerator::accept(const AstPointerType& /*ast*/) -> Result {
    return notImplemented();
}

auto IrGenerator::accept(const AstReferenceType& /*ast*/) -> Result {
    return notImplemented();
}
