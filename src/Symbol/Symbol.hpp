//
// Created by Albert Varaksin on 06/07/2020.
//
#pragma once
#include "Ast/ValueFlags.hpp"

namespace lbc {
class TypeProxy;
struct AstDecl;

class Symbol final {
public:
    NO_COPY_AND_MOVE(Symbol)

    explicit Symbol(llvm::StringRef name, TypeProxy* proxy = nullptr) noexcept
    : m_name{ name }, m_typeProxy{ proxy }, m_alias{ "" } {}

    ~Symbol() noexcept = default;

    [[nodiscard]] inline ValueFlags& getFlags() noexcept { return m_flags; }
    void inline setFlags(ValueFlags flags) noexcept { m_flags = flags; }

    [[nodiscard]] Symbol* getParent() const noexcept { return m_parent; }
    void setParent(Symbol* parent) noexcept { m_parent = parent; }

    [[nodiscard]] unsigned int getIndex() const noexcept { return m_index; }
    void setIndex(unsigned int index) noexcept { m_index = index; }

    [[nodiscard]] TypeProxy* getTypeProxy() const noexcept { return m_typeProxy; }
    void setTypeProxy(TypeProxy* proxy) noexcept { m_typeProxy = proxy; }

    [[nodiscard]] llvm::StringRef name() const noexcept { return m_name; }

    [[nodiscard]] llvm::Value* getLlvmValue() const noexcept { return m_llvmValue; }
    void setLlvmValue(llvm::Value* value) noexcept { m_llvmValue = value; }

    [[nodiscard]] llvm::StringRef alias() const noexcept { return m_alias; }
    void setAlias(llvm::StringRef alias) noexcept { m_alias = alias; }

    [[nodiscard]] AstDecl* getDecl() const noexcept { return m_decl; }
    void setDecl(AstDecl* decl) noexcept { m_decl = decl; }

    [[nodiscard]] llvm::StringRef identifier() const noexcept {
        if (m_alias.empty()) {
            return m_name;
        }
        return m_alias;
    }

    [[nodiscard]] llvm::GlobalValue::LinkageTypes getLlvmLinkage() const noexcept {
        if (m_flags.external) {
            return llvm::GlobalValue::LinkageTypes::ExternalLinkage;
        }
        return llvm::GlobalValue::LinkageTypes::InternalLinkage;
    }

    // Make vanilla new/delete illegal.
    void* operator new(size_t) = delete;
    void operator delete(void*) = delete;

    // Allow placement new
    void* operator new(size_t /*size*/, void* ptr) {
        assert(ptr); // NOLINT
        return ptr;
    }

private:
    const llvm::StringRef m_name;
    TypeProxy* m_typeProxy;

    AstDecl* m_decl = nullptr;
    llvm::StringRef m_alias;
    llvm::Value* m_llvmValue = nullptr;
    Symbol* m_parent = nullptr;
    unsigned int m_index = 0;
    ValueFlags m_flags{};
};

} // namespace lbc
