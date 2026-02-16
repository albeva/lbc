//
// Created by Albert Varaksin on 13/02/2026.
//
#pragma once
#include "pch.hpp"
#include "Diag/DiagEngine.hpp"
namespace lbc {
class Context;

/**
 * Satisfied by any type that exposes a `getContext()` method
 * returning `Context&`. Used by LogProvider to access the
 * diagnostic engine through deducing this.
 */
template <typename T>
concept ContextAware = requires(T& obj) {
    { obj.getContext() } -> std::same_as<Context&>;
};

class Context final {
public:
    NO_COPY_AND_MOVE(Context)
    Context();

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
     * Allocate object in the context and construct it.
     * @tparam T object to create
     * @tparam Args arguments to pass to T constructor
     */
    template <typename T, typename... Args>
    [[nodiscard]] auto create(Args&&... args) -> T* {
        T* obj = static_cast<T*>(allocate(sizeof(T), alignof(T)));
        return std::construct_at<T>(obj, std::forward<Args>(args)...);
    }

    /**
     * Get LLVM SourceMgr
     */
    [[nodiscard]] auto getSourceMgr() -> llvm::SourceMgr& { return m_sourceMgr; }

    /**
     * Get diagnostics engine
     */
    [[nodiscard]] auto getDiag() -> DiagEngine& { return m_diagEngine; }

private:
    llvm::SourceMgr m_sourceMgr;
    llvm::BumpPtrAllocator m_allocator;
    llvm::StringSet<llvm::BumpPtrAllocator> m_strings;
    DiagEngine m_diagEngine;
};

} // namespace lbc
