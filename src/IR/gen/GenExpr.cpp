//
// Created by Albert Varaksin on 08/03/2026.
//
#include "IrGenerator.hpp"
using namespace lbc::ir::gen;

auto IrGenerator::accept(const AstCastExpr& /*ast*/) -> Result {
    return notImplemented();
}

auto IrGenerator::accept(const AstVarExpr& /*ast*/) -> Result {
    return notImplemented();
}

auto IrGenerator::accept(const AstCallExpr& /*ast*/) -> Result {
    return notImplemented();
}

auto IrGenerator::accept(const AstLiteralExpr& /*ast*/) -> Result {
    return notImplemented();
}

auto IrGenerator::accept(const AstUnaryExpr& /*ast*/) -> Result {
    return notImplemented();
}

auto IrGenerator::accept(const AstBinaryExpr& /*ast*/) -> Result {
    return notImplemented();
}

auto IrGenerator::accept(const AstMemberExpr& /*ast*/) -> Result {
    return notImplemented();
}
