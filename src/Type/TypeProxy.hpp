//
// Created by Albert on 26/02/2022.
//
#pragma once

namespace lbc {
class TypeRoot;
struct AstDecl;

/**
 * Proxy object used by AST nodes
 * and symbols to late bind types
 */
class TypeProxy final {
public:
    NO_COPY_AND_MOVE(TypeProxy)
    constexpr TypeProxy() noexcept = default;
    ~TypeProxy() noexcept = default;

    constexpr explicit TypeProxy(const TypeRoot* type, AstDecl* decl = nullptr) noexcept
    : m_decl{ decl }, m_type{ type } {}

    // no new / delete. Must be allocated in the context
    void* operator new(size_t) = delete;
    void operator delete(void*) = delete;

    // Allow placement new
    void* operator new(size_t /*size*/, void* ptr) {
        assert(ptr); // NOLINT
        return ptr;
    }

    /// get Decl node (for UDT types)
    [[nodiscard]] constexpr AstDecl* getDecl() const noexcept { return m_decl; }

    /// Get type
    [[nodiscard]] const constexpr TypeRoot* getType() const noexcept { return m_type; }

    /// Set Type
    constexpr void setType(const TypeRoot* type) noexcept { m_type = type; }

private:
    AstDecl* m_decl = nullptr;
    const TypeRoot* m_type = nullptr;
};

} // namespace lbc