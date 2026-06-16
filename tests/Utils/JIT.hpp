//
// In-process ORC JIT used by the test harness to execute compiled modules.
// Lives in the test tree so the compiler itself never links ORC/ExecutionEngine.
//
#pragma once
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/Support/TargetSelect.h>
namespace lbc::test {

class JIT final {
public:
    static auto create() -> llvm::Expected<JIT> {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();
        auto jit = llvm::orc::LLJITBuilder().create();
        if (!jit) {
            return jit.takeError();
        }
        return JIT { std::move(*jit) };
    }

    [[nodiscard]] auto getDataLayout() const -> const llvm::DataLayout& {
        return m_jit->getDataLayout();
    }

    /** Bind @p name to the address of an existing function (e.g. redirect printf). */
    auto define(llvm::StringRef name, auto* addr) -> llvm::Error {
        llvm::orc::MangleAndInterner mangle { m_jit->getExecutionSession(), m_jit->getDataLayout() };
        const auto flags = llvm::JITSymbolFlags::Exported | llvm::JITSymbolFlags::Callable;
        const llvm::orc::SymbolMap symbols { { mangle(name), { llvm::orc::ExecutorAddr::fromPtr(addr), flags } } };
        return m_jit->getMainJITDylib().define(llvm::orc::absoluteSymbols(symbols));
    }

    auto addModule(llvm::orc::ThreadSafeModule module) -> llvm::Error {
        return m_jit->addIRModule(std::move(module));
    }

    auto lookup(llvm::StringRef name) -> llvm::Expected<llvm::orc::ExecutorAddr> {
        return m_jit->lookup(name);
    }

    auto initialize() -> llvm::Error {
        return m_jit->initialize(m_jit->getMainJITDylib());
    }

private:
    explicit JIT(std::unique_ptr<llvm::orc::LLJIT> jit)
    : m_jit { std::move(jit) } {}

    std::unique_ptr<llvm::orc::LLJIT> m_jit;
};

} // namespace lbc::test
