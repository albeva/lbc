//
// Created by Albert Varaksin on 13/02/2026.
//
#pragma once
#include "pch.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/TargetParser/Triple.h>
#include "CompileOptions.hpp"
#include "Diag/DiagEngine.hpp"
#include "Type/TypeFactory.hpp"
namespace lbc {
class Context;

/**
 * Satisfied by any type that exposes a `getContext()` method
 * returning `Context&`. Used by LogProvider to access the
 * diagnostic engine through deducing this.
 */
template<typename T>
concept ContextAware = requires(const T& obj) {
    { obj.getContext() } -> std::same_as<Context&>;
};

class Context final {
public:
    NO_COPY_AND_MOVE(Context)

    /** Construct a context that owns the (frozen) options for this compilation. */
    explicit Context(CompileOptions options = {});

    /**
     * Intern given string in a set and return unique, shared copy.
     *
     * @param string string to intern
     * @return interned copy of the string
     */
    [[nodiscard]] auto retain(llvm::StringRef string) -> llvm::StringRef;

    /**
     * Allocate memory, this memory is not expected to be deallocated
     */
    [[nodiscard]] auto allocate(const std::size_t bytes, const std::size_t alignment) -> void* {
        return m_allocator.Allocate(bytes, alignment);
    }

    /**
     * Allocate an uninitialized array of T.
     *
     * @tparam T The element type
     * @param count Number of elements
     * @return Span over the allocated buffer
     */
    template<typename T>
    [[nodiscard]] auto span(std::size_t count) -> std::span<T> {
        void* buffer = allocate(sizeof(T) * count, alignof(T)); // NOLINT(*-init-variables)
        return { static_cast<T*>(buffer), count };
    }

    /**
     * Allocate object in the context and construct it.
     * @tparam T object to create
     * @tparam Args arguments to pass to T constructor
     */
    template<typename T, typename... Args>
    [[nodiscard]] auto create(Args&&... args) -> T* {
        T* obj = static_cast<T*>(allocate(sizeof(T), alignof(T)));
        return std::construct_at<T>(obj, std::forward<Args>(args)...);
    }

    /**
     * Get LLVM SourceMgr
     */
    [[nodiscard]] auto getSourceMgr() -> llvm::SourceMgr& { return *m_sourceMgr; }

    /**
     * Get diagnostics engine
     */
    [[nodiscard]] auto getDiag() -> DiagEngine& { return m_diagEngine; }

    /**
     * Get the type factory
     */
    [[nodiscard]] auto getTypeFactory() -> TypeFactory& { return m_typeFactory; }

    /**
     * Get the (read-only) options driving this compilation
     */
    [[nodiscard]] auto getOptions() const -> const CompileOptions& { return m_options; }

    /**
     * Get the resolved target triple for this compilation
     */
    [[nodiscard]] auto getTriple() const -> const llvm::Triple& { return m_triple; }

    /**
     * Get the LLVM context owning this compilation's modules
     */
    [[nodiscard]] auto getLlvmContext() -> llvm::LLVMContext& { return *m_llvmContext; }

    /**
     * Create a temporary file with the given @p suffix and return its path. The
     * caller owns the file — typically by wrapping it in a temporary @ref
     * Artefact, which deletes it once consumed.
     *
     * @param suffix file-name suffix (without a leading dot), e.g. "bc"
     * @return absolute path to the new file, or empty on failure
     */
    [[nodiscard]] auto createTempFile(llvm::StringRef suffix) -> std::string;

    /**
     * Swap context
     *
     * This will initialize a new empty context object.
     *
     * @return current llvm context instance
     */
    [[nodiscard]] auto replaceContext(std::unique_ptr<llvm::LLVMContext> replacement) -> std::unique_ptr<llvm::LLVMContext>;

    /**
     * Swap SourceMgr
     *
     * A new empty SourceMgr instance will be initialized
     *
     * @return current source manager instance
     */
    [[nodiscard]] auto replaceSourceManager(std::unique_ptr<llvm::SourceMgr> replacement) -> std::unique_ptr<llvm::SourceMgr>;

private:
    const CompileOptions m_options;
    llvm::Triple m_triple;
    std::unique_ptr<llvm::LLVMContext> m_llvmContext;
    std::unique_ptr<llvm::SourceMgr> m_sourceMgr;
    llvm::BumpPtrAllocator m_allocator;
    llvm::StringSet<llvm::BumpPtrAllocator> m_strings;
    DiagEngine m_diagEngine;
    TypeFactory m_typeFactory;
};

} // namespace lbc
