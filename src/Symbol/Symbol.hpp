//
// Created by Albert Varaksin on 06/07/2020.
//
#pragma once
#include "pch.hpp"
#include "Ast/ValueFlags.hpp"

namespace lbc {
class TypeRoot;
struct AstDecl;
class SymbolTable;

class Symbol final {
public:
    NO_COPY_AND_MOVE(Symbol)

    struct StateFlags final {
        // Symbol is being analysed
        uint8_t beingDefined : 1;
        // Symbol is declared and usable in expressions
        uint8_t declared : 1;
    };

    explicit Symbol(
        llvm::StringRef name,
        SymbolTable* table,
        const TypeRoot* type,
        AstDecl* decl) noexcept
    : m_name{ name }, m_table{ table }, m_type{ type }, m_decl{ decl }, m_alias{ "" } {}

    ~Symbol() noexcept = default;

    [[nodiscard]] inline ValueFlags& valueFlags() noexcept { return m_flags; }
    [[nodiscard]] inline StateFlags& stateFlags() noexcept { return m_state; }

    [[nodiscard]] unsigned int getIndex() const noexcept { return m_index; }
    inline void setIndex(unsigned index) noexcept { m_index = index; }

    [[nodiscard]] inline const TypeRoot* getType() const noexcept { return m_type; }
    inline void setType(const TypeRoot* type) noexcept { m_type = type; }

    [[nodiscard]] llvm::StringRef name() const noexcept { return m_name; }

    [[nodiscard]] llvm::Value* getLlvmValue() const noexcept { return m_llvmValue; }
    void setLlvmValue(llvm::Value* value) noexcept { m_llvmValue = value; }

    [[nodiscard]] llvm::StringRef alias() const noexcept { return m_alias; }
    void setAlias(llvm::StringRef alias) noexcept { m_alias = alias; }

    [[nodiscard]] AstDecl* getDecl() const noexcept { return m_decl; }
    void setDecl(AstDecl* decl) noexcept { m_decl = decl; }

    [[nodiscard]] SymbolTable* getSymbolTable() const noexcept { return m_table; }

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
    SymbolTable* m_table;
    const TypeRoot* m_type;
    AstDecl* m_decl;

    llvm::StringRef m_alias;
    llvm::Value* m_llvmValue = nullptr;
    unsigned m_index = 0;
    ValueFlags m_flags{};
    StateFlags m_state{};
};

} // namespace lbc
