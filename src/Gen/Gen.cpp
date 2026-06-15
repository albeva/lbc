//
// Created by Albert Varaksin on 15/06/2026.
//
#include "Generator.hpp"
#include "IR/lib/BasicBlock.hpp"
#include "IR/lib/Function.hpp"
#include "IR/lib/Literal.hpp"
#include "IR/lib/Module.hpp"
#include "IR/lib/Variable.hpp"
#include "Symbol/Symbol.hpp"
#include "Type/Type.hpp"
using namespace lbc;
using namespace lbc::gen;

Generator::Generator()
: m_builder(m_llvm) {}

auto Generator::generate(const ir::lib::Module& module) -> llvm::Module& {
    m_module = std::make_unique<llvm::Module>("lbc", m_llvm);

    // Declare all defined functions first so calls between them resolve, then
    // lower their bodies.
    for (const auto& fn : module.getFunctions()) {
        std::ignore = function(fn);
    }
    for (const auto& fn : module.getFunctions()) {
        lowerFunction(fn);
    }

    // Top-level code becomes the body of `main`.
    lowerGlobalInit(module);

    return *m_module;
}

// =============================================================================
// Walk
// =============================================================================

void Generator::lowerFunction(const ir::lib::Function& fn) {
    m_function = function(fn);

    // Pre-create all blocks so branches can target forward blocks.
    for (const auto& block : fn.getBlocks()) {
        block.setLlvm(llvm::BasicBlock::Create(m_llvm, block.getName(), m_function));
    }
    for (const auto& block : fn.getBlocks()) {
        lowerBlock(block);
    }

    m_function = nullptr;
}

void Generator::lowerGlobalInit(const ir::lib::Module& module) {
    auto* type = llvm::FunctionType::get(m_builder.getInt32Ty(), false);
    m_function = llvm::Function::Create(type, llvm::Function::ExternalLinkage, "main", *m_module);

    auto* entry = llvm::BasicBlock::Create(m_llvm, "entry", m_function);
    m_builder.SetInsertPoint(entry);
    for (const auto& instr : module.getGlobalInitBlock()->getBody()) {
        lowerInstruction(instr);
    }
    m_builder.CreateRet(m_builder.getInt32(0));

    m_function = nullptr;
}

void Generator::lowerBlock(const ir::lib::BasicBlock& block) {
    m_builder.SetInsertPoint(llvm::cast<llvm::BasicBlock>(block.getLlvm()));
    for (const auto& instr : block.getBody()) {
        lowerInstruction(instr);
    }
}

// =============================================================================
// Values
// =============================================================================

auto Generator::value(const ir::lib::Value* val) -> llvm::Value* {
    if (const auto* lit = llvm::dyn_cast<ir::lib::Literal>(val)) {
        return literal(*lit);
    }
    // A variable used as a value is a read: load through its storage.
    if (llvm::isa<ir::lib::Variable>(val)) {
        return m_builder.CreateLoad(lowerType(val->getType()), val->getLlvm());
    }
    return val->getLlvm();
}

auto Generator::address(const ir::lib::NamedValue* val) -> llvm::Value* {
    // Variables map to their alloca; temporaries already hold a pointer.
    return val->getLlvm();
}

auto Generator::function(const ir::lib::Function& fn) -> llvm::Function* {
    if (auto* existing = fn.getLlvm()) {
        return llvm::cast<llvm::Function>(existing);
    }
    auto* type = lowerFunctionType(fn.getSymbol()->getType());
    auto* func = llvm::Function::Create(type, llvm::Function::ExternalLinkage, fn.getName(), *m_module);
    fn.setLlvm(func);
    return func;
}

auto Generator::literal(const ir::lib::Literal& lit) -> llvm::Constant* {
    const auto& val = lit.getValue();
    auto* type = lowerType(lit.getType());

    if (val.isBool()) {
        return llvm::ConstantInt::getBool(type, val.get<bool>());
    }
    if (val.isIntegral()) {
        return llvm::ConstantInt::get(type, val.get<std::uint64_t>(), isSigned(lit.getType()));
    }
    if (val.isFloatingPoint()) {
        return llvm::ConstantFP::get(type, val.get<double>());
    }
    if (val.isString()) {
        return m_builder.CreateGlobalString(val.get<llvm::StringRef>());
    }
    if (val.isNull()) {
        return llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(type));
    }
    std::unreachable();
}
