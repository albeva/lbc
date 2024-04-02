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
    : m_llJit(std::move(llJit))
    {
    }

    static llvm::Expected<std::unique_ptr<JIT>> Create() {
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

//class JIT {
//private:
//    std::unique_ptr<llvm::orc::ExecutionSession> m_executionSession;
//
//    llvm::DataLayout m_dataLayout;
//    llvm::orc::MangleAndInterner m_mangler;
//
//    llvm::orc::RTDyldObjectLinkingLayer m_objectLayer;
//    llvm::orc::IRCompileLayer m_compileLayer;
//
//    llvm::orc::JITDylib& m_mainJD;
//
//public:
//    JIT(
//        std::unique_ptr<llvm::orc::ExecutionSession> ES,
//        llvm::orc::JITTargetMachineBuilder JTMB,
//        llvm::DataLayout DL)
//    : m_executionSession(std::move(ES)),
//      m_dataLayout(std::move(DL)),
//      m_mangler(*this->m_executionSession, this->m_dataLayout),
//      m_objectLayer(*this->m_executionSession, []() { return std::make_unique<llvm::SectionMemoryManager>(); }),
//      m_compileLayer(*this->m_executionSession, m_objectLayer, std::make_unique<llvm::orc::ConcurrentIRCompiler>(std::move(JTMB))),
//      m_mainJD(this->m_executionSession->createBareJITDylib("<main>")) {
//        m_mainJD.addGenerator(
//            cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
//                DL.getGlobalPrefix())));
//    }
//
//    ~JIT() {
//        if (auto Err = m_executionSession->endSession())
//            m_executionSession->reportError(std::move(Err));
//    }
//
//    static llvm::Expected<std::unique_ptr<JIT>> Create() {
//        auto EPC = llvm::orc::SelfExecutorProcessControl::Create();
//        if (!EPC)
//            return EPC.takeError();
//
//        auto ES = std::make_unique<llvm::orc::ExecutionSession>(std::move(*EPC));
//
//        llvm::orc::JITTargetMachineBuilder JTMB(
//            ES->getExecutorProcessControl().getTargetTriple());
//
//        auto DL = JTMB.getDefaultDataLayoutForTarget();
//        if (!DL)
//            return DL.takeError();
//
//        return std::make_unique<JIT>(std::move(ES), std::move(JTMB), std::move(*DL));
//    }
//
//    const llvm::DataLayout& getDataLayout() const { return m_dataLayout; }
//
//    llvm::orc::JITDylib& getMainJITDylib() { return m_mainJD; }
//
//    llvm::Error addModule(llvm::orc::ThreadSafeModule TSM, llvm::orc::ResourceTrackerSP RT = nullptr) {
//        if (!RT)
//            RT = m_mainJD.getDefaultResourceTracker();
//        return m_compileLayer.add(RT, std::move(TSM));
//    }
//
//    llvm::Expected<llvm::orc::ExecutorSymbolDef> lookup(llvm::StringRef Name) {
//        return m_executionSession->lookup({ &m_mainJD }, m_mangler(Name.str()));
//    }
//};

} // namespace lbc