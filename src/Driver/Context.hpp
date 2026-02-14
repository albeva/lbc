//
// Created by Albert Varaksin on 13/02/2026.
//
#pragma once
#include "pch.hpp"
namespace lbc {

class Context final {
public:
    Context() = default;

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

private:
    llvm::SourceMgr m_sourceMgr;
    llvm::BumpPtrAllocator m_allocator;
    llvm::StringSet<llvm::BumpPtrAllocator> m_strings;
};

} // namespace lbc
