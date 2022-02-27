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

    TypeProxy() noexcept = default;
    ~TypeProxy() noexcept = default;

    explicit TypeProxy(const TypeRoot* type, AstDecl* decl = nullptr) noexcept
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
    [[nodiscard]] AstDecl* getDecl() const noexcept { return m_decl; }

    /// Get type
    [[nodiscard]] const TypeRoot* getType() const noexcept { return m_type; }

    /// Set Type
    void setType(const TypeRoot* type) noexcept { m_type = type; }

private:
    AstDecl* m_decl = nullptr;
    const TypeRoot* m_type = nullptr;
};

} // namespace lbc