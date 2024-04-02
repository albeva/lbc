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

    static llvm::Expected<std::unique_ptr<JIT>> create() {
        auto llJit = llvm::orc::LLJITBuilder().create();
        if (!llJit) {
            return llJit.takeError();
        }

        return std::make_unique<JIT>(std::move(*llJit));
    }

    [[nodiscard]] const llvm::DataLayout& getDataLayout() const {
        return m_llJit->getDataLayout();
    }

    llvm::Error addModule(llvm::orc::ThreadSafeModule module) {
        return m_llJit->addIRModule(std::move(module));
    }

    llvm::Expected<llvm::orc::ExecutorAddr> lookup(llvm::StringRef name) {
        return m_llJit->lookup(name);
    }

private:
    std::unique_ptr<llvm::orc::LLJIT> m_llJit;
};

} // namespace lbc