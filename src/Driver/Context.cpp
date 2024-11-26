//
// Created by Albert Varaksin on 18/04/2021.
//
#include "Context.hpp"
#include "CompileOptions.hpp"
#include "Diag/DiagnosticEngine.hpp"
#include "Driver/Toolchain/Toolchain.hpp"
#include "JIT.hpp"
#include "Type/Type.hpp"
#include <llvm-c/Target.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/TargetParser/Host.h>
using namespace lbc;

struct Context::Pimpl {
    explicit Pimpl(Context& context)
    : diag{ context },
      toolchain{ context } {}

    DiagnosticEngine diag;
    Toolchain toolchain;
    std::optional<llvm::DataLayout> dataLayout{};
};

Context::Context(CompileOptions& options)
: m_pimpl{ std::make_unique<Pimpl>(*this) },
  m_options{ options },
  m_diag{ m_pimpl->diag },
  m_toolchain{ m_pimpl->toolchain },
  m_triple{ llvm::sys::getDefaultTargetTriple() }
{
    switch (m_options.getCompilationMode()) {
    case CompileOptions::CompilationMode::Bit32:
        m_triple = m_triple.get32BitArchVariant();
        break;
    case CompileOptions::CompilationMode::Bit64:
        m_triple = m_triple.get64BitArchVariant();
        break;
    default:
        llvm_unreachable("Unknown compilation mode");
    }
}

Context::~Context() = default;

void Context::reset() {
    m_jit.reset();

    for (auto& func : funcTypes) {
        func->reset();
    }

    for (auto& ptr : ptrTypes) {
        ptr->reset();
    }

    funcTypes.clear();
    ptrTypes.clear();

    m_options.reset();
    m_sourceMgr = llvm::SourceMgr{};
    m_retainedStrings.clear();
    m_imports.clear();
    m_allocator.Reset();
}

auto Context::getJIT() noexcept -> JIT& {
    if (!m_jit) {
        LLVMInitializeNativeTarget();
        LLVMInitializeNativeAsmPrinter();
        LLVMInitializeNativeAsmParser();
        m_jit = llvm::ExitOnError()(JIT::create());
    }
    return *m_jit;
}

[[nodiscard]] auto Context::getDataLayout() noexcept -> const llvm::DataLayout& {
    if (m_jit) {
        return m_jit->getDataLayout();
    }

    if (!m_pimpl->dataLayout) {
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();

        std::string error{};
        const llvm::Target* target = llvm::TargetRegistry::lookupTarget(getTriple().str(), error);
        if (target == nullptr) {
            fatalError("Failed to find the target for triple: " + error);
        }

        auto level = [&]() -> llvm::CodeGenOptLevel {
            switch (m_options.getOptimizationLevel()) {
            case CompileOptions::OptimizationLevel::O0:
                return llvm::CodeGenOptLevel::None;
            case CompileOptions::OptimizationLevel::OS:
                return llvm::CodeGenOptLevel::Default;
            case CompileOptions::OptimizationLevel::O1:
                return llvm::CodeGenOptLevel::Less;
            case CompileOptions::OptimizationLevel::O2:
                return llvm::CodeGenOptLevel::Default;
            case CompileOptions::OptimizationLevel::O3:
                return llvm::CodeGenOptLevel::Aggressive;
            default:
                llvm_unreachable("invalid optimization level");
            }
        }();

        std::unique_ptr<llvm::TargetMachine> machine{ target->createTargetMachine(
            getTriple().str(),
            /* cpu */ "",
            /* features*/ "",
            llvm::TargetOptions(),
            /*Reloc::Model*/ std::nullopt,
            /*CodeModel::Model*/ std::nullopt,
            /*CodeGenOptLevel*/ level
        ) };

        if (!machine) {
            fatalError("Failed to create target machine");
        }

        m_pimpl->dataLayout = machine->createDataLayout();
    }

    return m_pimpl->dataLayout.value();
}

auto Context::retainCopy(llvm::StringRef str) -> llvm::StringRef {
    return m_retainedStrings.insert(str).first->first();
}

auto Context::import(llvm::StringRef module) -> bool {
    return m_imports.insert(module).second;
}
