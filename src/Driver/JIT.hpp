//
// Created by Albert Varaksin on 02/04/2024.
//
#pragma once
#include "pch.hpp"
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
namespace lbc {

class JIT final {
public:
    explicit JIT(std::unique_ptr<llvm::orc::LLJIT> llJit)
    : m_llJit(std::move(llJit)) {
    }

    static auto create() -> llvm::Expected<std::unique_ptr<JIT>> {
        auto llJit = llvm::orc::LLJITBuilder().create();
        if (!llJit) {
            return llJit.takeError();
        }

        return std::make_unique<JIT>(std::move(*llJit));
    }

    [[nodiscard]] auto getDataLayout() const -> const llvm::DataLayout& {
        return m_llJit->getDataLayout();
    }

    auto define(llvm::StringRef name, auto* addr, llvm::JITSymbolFlags flags) -> llvm::Error {
        llvm::orc::MangleAndInterner mangler{ m_llJit->getExecutionSession(), m_llJit->getDataLayout() };

        return m_llJit->getMainJITDylib().define(llvm::orc::absoluteSymbols(llvm::orc::SymbolMap{
            { mangler(name), llvm::orc::ExecutorSymbolDef(llvm::orc::ExecutorAddr::fromPtr(addr), flags) } }));
    }

    auto addModule(llvm::orc::ThreadSafeModule module) -> llvm::Error {
        return m_llJit->addIRModule(std::move(module));
    }

    auto lookup(llvm::StringRef name) -> llvm::Expected<llvm::orc::ExecutorAddr> {
        return m_llJit->lookup(name);
    }

    auto initialize() noexcept -> llvm::Error {
        return m_llJit->initialize(m_llJit->getMainJITDylib());
    }

    auto deinitialize() noexcept -> llvm::Error {
        return m_llJit->deinitialize(m_llJit->getMainJITDylib());
    }

private:
    std::unique_ptr<llvm::orc::LLJIT> m_llJit;
};

} // namespace lbc