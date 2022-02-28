//
// Created by Albert on 26/02/2022.
//
#pragma once

namespace lbc {
class TypeRoot;
class Context;
struct AstDecl;

/**
 * Proxy object used by AST nodes
 * and symbols to late bind types
 */
class TypeProxy final {
public:
    NO_COPY_AND_MOVE(TypeProxy)
    constexpr TypeProxy() noexcept = default;
    constexpr ~TypeProxy() noexcept = default;

    constexpr explicit TypeProxy(const TypeRoot* type) noexcept
    : m_storage{ type } {}

    constexpr explicit TypeProxy(TypeProxy* proxy) noexcept
    : m_storage{ proxy } {}

    // no new / delete. Must be allocated in the context
    void* operator new(size_t) = delete;
    void operator delete(void*) = delete;

    // Allow placement new
    void* operator new(size_t /*size*/, void* ptr) {
        assert(ptr); // NOLINT
        return ptr;
    }

    /// Get type
    [[nodiscard]] const TypeRoot* getType() noexcept;

    /// Set Type
    void setType(const TypeRoot* type) noexcept {
        assert(!hasValue() && "Proxy must be empty when setting a type"); // NOLINT
        m_storage = type;
    }

    /// Get nested type proxy
    [[nodiscard]] TypeProxy* getNestedProxy() const noexcept;

    /// Set nested proxy
    void setNestedProxy(TypeProxy* proxy) noexcept {
        assert(!hasValue() && "Proxy must be empty when setting a proxy"); // NOLINT
        m_storage = proxy;
    }

    /// Set indirection level (pointer)
    void setDereference(int dereference, Context* context) noexcept {
        assert(context != nullptr && "Context must be provided"); // NOLINT
        m_context = context;
        m_dereference = dereference;
    }

    /// Return true if this proxy holds a nested proxy or a type
    [[nodiscard]] constexpr bool hasValue() const noexcept {
        return not std::holds_alternative<std::monostate>(m_storage);
    }

private:
    [[nodiscard]] const TypeRoot* getBaseType() noexcept;

    using Storage = std::variant<std::monostate, const TypeRoot*, TypeProxy*>;
    Storage m_storage{ std::monostate() };
    Context* m_context = nullptr;
    int m_dereference = 0;
};

} // namespace lbc