//
// Created by Albert Varaksin on 08/03/2026.
//
#include "IrGenerator.hpp"
#include "IR/lib/BasicBlock.hpp"
#include "IR/lib/Module.hpp"
#include "IR/lib/ScopedBlock.hpp"
#include "IR/lib/Temporary.hpp"
using namespace lbc::ir::gen;

IrGenerator::IrGenerator(Context& context)
: Builder(context) {
}

IrGenerator::~IrGenerator() = default;

auto IrGenerator::generate(const AstModule& ast) -> DiagResult<lib::Module*> {
    TRY(accept(ast));
    return m_module;
}

auto IrGenerator::accept(const AstModule& ast) -> Result {
    m_module = getContext().create<lib::Module>();
    return accept(*ast.getStmtList());
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

void IrGenerator::emit(lib::Instruction* instr) {
    m_block->getBody().push_back(instr);
}

auto IrGenerator::createBlock(llvm::StringRef name) -> lib::BasicBlock* {
    auto* block = getContext().create<lib::BasicBlock>(getContext(), name.str());
    m_function->getBlocks().push_back(block);
    return block;
}

void IrGenerator::setInsertPoint(lib::BasicBlock* block) {
    m_block = block;
}

auto IrGenerator::isTerminated() const -> bool {
    if (m_block == nullptr || m_block->getBody().empty()) {
        return false;
    }
    return llvm::isa<lib::IrTerminator>(m_block->getBody().back());
}

auto IrGenerator::createTemporary(const Type* type) -> lib::Temporary* {
    return getContext().create<lib::Temporary>(std::to_string(m_tempCounter++), type);
}
