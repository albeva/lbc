//
// Created by Albert Varaksin on 08/03/2026.
//
#include "IrGenerator.hpp"
using namespace lbc::ir::gen;

IrGenerator::IrGenerator(Context& context)
: Builder(context) {
}

IrGenerator::~IrGenerator() = default;

auto IrGenerator::generate(const AstModule& ast) -> Result {
    return accept(ast);
}

auto IrGenerator::accept(const AstModule& ast) -> Result {
    return accept(*ast.getStmtList());
}
