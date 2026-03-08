//
// Created by Albert Varaksin on 08/03/2026.
//
#include "IrGenerator.hpp"
using namespace lbc::ir::gen;

IrGenerator::IrGenerator(Context& context)
: Builder(context)
, m_module(nullptr) {
}

IrGenerator::~IrGenerator() = default;

auto IrGenerator::generate(const AstModule& ast) -> DiagResult<lib::Module*> {
    TRY(accept(ast));
    return m_module;
}

auto IrGenerator::accept(const AstModule& ast) -> Result {
    return accept(*ast.getStmtList());
}
