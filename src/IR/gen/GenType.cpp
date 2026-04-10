//
// Created by Albert Varaksin on 08/03/2026.
//
#include "IrGenerator.hpp"
using namespace lbc::ir::gen;

auto IrGenerator::accept(const AstBuiltInType& /*ast*/) -> Result {
    // Types are resolved during semantic analysis; nothing to emit.
    return {};
}

auto IrGenerator::accept(const AstPointerType& /*ast*/) -> Result {
    // Types are resolved during semantic analysis; nothing to emit.
    return {};
}

auto IrGenerator::accept(const AstReferenceType& /*ast*/) -> Result {
    // Types are resolved during semantic analysis; nothing to emit.
    return {};
}
