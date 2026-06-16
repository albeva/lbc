//
// Created by Albert Varaksin on 15/06/2026.
//
#include <llvm/IR/GlobalVariable.h>
#include "Driver/Context.hpp"
#include "Generator.hpp"
#include "IR/lib/BasicBlock.hpp"
#include "IR/lib/Function.hpp"
#include "IR/lib/Instructions.hpp"
#include "IR/lib/Literal.hpp"
#include "IR/lib/Module.hpp"
#include "IR/lib/Variable.hpp"
#include "Symbol/Symbol.hpp"
#include "Type/Type.hpp"
using namespace lbc;
using namespace lbc::gen;

Generator::Generator(Context& context)
: m_context(context)
, m_llvm(context.getLlvmContext())
, m_builder(m_llvm) {}

auto Generator::generate(const ir::lib::Module& module) -> std::unique_ptr<llvm::Module> {
    m_module = std::make_unique<llvm::Module>("lbc", m_llvm);
    m_module->setTargetTriple(m_context.getTriple());

    // Materialise module-scope globals before anything references them: their
    // initialiser stores live in the global-init block (lowered into `main`) and
    // function bodies may read them.
    for (const auto* decl : module.getDeclarations()) {
        lowerGlobal(llvm::cast<ir::lib::VarInstr>(*decl));
    }

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

    return std::move(m_module);
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

    // Materialise the parameters into the entry block before the body runs.
    bindParameters(fn);

    for (const auto& block : fn.getBlocks()) {
        lowerBlock(block);
    }

    m_function = nullptr;
}

void Generator::bindParameters(const ir::lib::Function& fn) {
    const auto params = fn.getParams();
    if (params.empty()) {
        return;
    }

    // Parameters get the same alloca-backed storage as locals: allocate a slot
    // for each, copy the incoming argument into it, and point the IR variable at
    // the slot so body reads and writes load/store through it uniformly.
    m_builder.SetInsertPoint(llvm::cast<llvm::BasicBlock>(fn.getBlocks().front().getLlvm()));
    std::size_t index = 0;
    for (auto& arg : m_function->args()) {
        const auto* param = params[index++];
        auto* slot = m_builder.CreateAlloca(lowerType(param->getType()), nullptr, param->getName());
        m_builder.CreateStore(&arg, slot);
        param->setLlvm(slot);
    }
}

void Generator::lowerGlobal(const ir::lib::VarInstr& var) {
    // A module-scope `dim` becomes an LLVM global. It is zero-initialised so the
    // global is a definition; any user initialiser runs as a store in the
    // global-init block (lowered into `main`), uniformly for constant and runtime
    // values. The module takes ownership of the GlobalVariable on construction.
    auto* type = lowerType(var.getType());
    auto* global = new llvm::GlobalVariable(
        *m_module,
        type,
        /*isConstant*/ false,
        llvm::GlobalValue::InternalLinkage,
        llvm::Constant::getNullValue(type),
        var.getResult()->getName()
    );
    var.getResult()->setLlvm(global);
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
