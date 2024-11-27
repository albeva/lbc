//
// Created by Albert Varaksin on 18/04/2021.
//
#pragma once
#include "pch.hpp"
#include "llvm/Support/Allocator.h"

namespace llvm {
class DataLayout;
}

namespace lbc {
struct CompileOptions;
class TypeFunction;
class TypePointer;
class DiagnosticEngine;
class Toolchain;
class JIT;

/**
 * Context holds various data and memory allocations required for the compilation process.
 * While it is not thread safe, as long as no more than 1 thread accesses it, it acts
 * similar to `thread_local` storage.
 */
class Context final {
public:
    NO_COPY_AND_MOVE(Context)

    explicit Context(CompileOptions& options);
    ~Context();

    void reset();

    [[nodiscard]] auto getOptions() const -> const CompileOptions& { return m_options; }
    [[nodiscard]] auto getDiag() -> DiagnosticEngine& { return m_diag; }
    [[nodiscard]] auto getToolchain() -> Toolchain& { return m_toolchain; }
    [[nodiscard]] auto getTriple() -> llvm::Triple& { return m_triple; }
    [[nodiscard]] auto getSourceMrg() -> llvm::SourceMgr& { return m_sourceMgr; }
    [[nodiscard]] auto getLlvmContext() -> llvm::LLVMContext& { return m_llvmContext; }
    [[nodiscard]] auto getJIT() noexcept -> JIT&;
    [[nodiscard]] auto getDataLayout() noexcept -> const llvm::DataLayout&;

    /**
     * Retain a copy of the string in the context and return a llvm::StringRef that
     * we can pass around safely without worry of it expiring (as long as context lives)
     * @param str string to retain
     */
    [[nodiscard]] auto retainCopy(llvm::StringRef str) -> llvm::StringRef;

    /**
     * Store imported modules
     * @return true if module is newly added, false otherwise
     */
    [[nodiscard]] auto import(llvm::StringRef module) -> bool;

    /**
     * Allocate memory, this memory is not expected to be deallocated
     */
    auto allocate(size_t bytes, unsigned alignment) -> void* {
        return m_allocator.Allocate(bytes, alignment);
    }

    /**
     * Allocate object in the context and construct it.
     * @tparam T object to create
     * @tparam Args arguments to pass to T constructor
     */
    template <typename T, typename... Args>
    auto create(Args&&... args) -> T* {
        T* res = static_cast<T*>(allocate(sizeof(T), alignof(T)));
        new (res) T(std::forward<Args>(args)...);
        return res;
    }

    /**
     * Get all function types
     *
     * @return vector of types
     */
    auto getFuncTypes() -> auto& {
        return funcTypes;
    }

    /**
     * Get all pointer types
     *
     * @return vector of types
     */
    auto getPtrTypes() -> auto& {
        return ptrTypes;
    }

private:
    struct Pimpl;

    std::vector<TypeFunction*> funcTypes;
    std::vector<TypePointer*> ptrTypes;
    std::unique_ptr<Pimpl> m_pimpl;
    CompileOptions& m_options;

    DiagnosticEngine& m_diag;
    Toolchain& m_toolchain;

    llvm::Triple m_triple;
    llvm::SourceMgr m_sourceMgr;
    llvm::LLVMContext m_llvmContext;

    llvm::StringSet<> m_retainedStrings;
    llvm::StringSet<> m_imports;

    std::unique_ptr<JIT> m_jit;

    // Allocations
    llvm::BumpPtrAllocator m_allocator;
};

} // namespace lbc
