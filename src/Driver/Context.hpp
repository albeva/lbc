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

    explicit Context(const CompileOptions& options);
    ~Context();

    [[nodiscard]] const CompileOptions& getOptions() const { return m_options; }
    [[nodiscard]] DiagnosticEngine& getDiag() { return m_diag; }
    [[nodiscard]] Toolchain& getToolchain() { return m_toolchain; }
    [[nodiscard]] llvm::Triple& getTriple() { return m_triple; }
    [[nodiscard]] llvm::SourceMgr& getSourceMrg() { return m_sourceMgr; }
    [[nodiscard]] llvm::LLVMContext& getLlvmContext() { return m_llvmContext; }
    [[nodiscard]] JIT& getJIT() noexcept;
    [[nodiscard]] const llvm::DataLayout& getDataLayout() noexcept;

    /**
     * Retain a copy of the string in the context and return a llvm::StringRef that
     * we can pass around safely without worry of it expiring (as long as context lives)
     * @param str string to retain
     */
    [[nodiscard]] llvm::StringRef retainCopy(llvm::StringRef str);

    /**
     * Store imported modules
     * @return true if module is newly added, false otherwise
     */
    [[nodiscard]] bool import(llvm::StringRef module);

    /**
     * Allocate memory, this memory is not expected to be deallocated
     */
    void* allocate(size_t bytes, unsigned alignment) {
        return m_allocator.Allocate(bytes, alignment);
    }

    /**
     * Allocate object in the context and construct it.
     * @tparam T object to create
     * @tparam Args arguments to pass to T constructor
     */
    template<typename T, typename... Args>
    T* create(Args&&... args) {
        T* res = static_cast<T*>(allocate(sizeof(T), alignof(T)));
        new (res) T(std::forward<Args>(args)...);
        return res;
    }

    llvm::SmallVector<TypeFunction*> funcTypes;
    llvm::SmallVector<TypePointer*> ptrTypes;

private:
    struct Pimpl;
    std::unique_ptr<Pimpl> m_pimpl;
    const CompileOptions& m_options;

    DiagnosticEngine& m_diag;
    Toolchain& m_toolchain;

    llvm::Triple m_triple;
    llvm::SourceMgr m_sourceMgr{};
    llvm::LLVMContext m_llvmContext{};

    llvm::StringSet<> m_retainedStrings{};
    llvm::StringSet<> m_imports;

    std::unique_ptr<JIT> m_jit;

    // Allocations
    llvm::BumpPtrAllocator m_allocator;
};

} // namespace lbc
