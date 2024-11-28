//
// Created by Albert Varaksin on 06/07/2020.
//
#pragma once
#include "pch.hpp"
#include "Ast/ValueFlags.hpp"
#include "Lexer/Token.hpp"

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

    Symbol(
        llvm::StringRef name,
        SymbolTable* table,
        const TypeRoot* type,
        AstDecl* decl
    )
    : m_name { name }
    , m_table { table }
    , m_type { type }
    , m_decl { decl }
    , m_alias { "" } {
    }

    ~Symbol() = default;

    [[nodiscard]] auto valueFlags() -> ValueFlags& { return m_flags; }
    [[nodiscard]] auto stateFlags() -> StateFlags& { return m_state; }

    [[nodiscard]] auto getIndex() const -> unsigned int { return m_index; }
    void setIndex(unsigned index) { m_index = index; }

    [[nodiscard]] auto getType() const -> const TypeRoot* { return m_type; }
    void setType(const TypeRoot* type) { m_type = type; }

    [[nodiscard]] auto name() const -> llvm::StringRef { return m_name; }

    [[nodiscard]] auto getConstantValue() const -> const std::optional<Token::Value>& { return m_constantValue; }
    [[nodiscard]] auto setConstantValue(const std::optional<Token::Value>& value) { m_constantValue = value; }

    [[nodiscard]] auto getLlvmValue() const -> llvm::Value* { return m_llvmValue; }
    void setLlvmValue(llvm::Value* value) { m_llvmValue = value; }

    [[nodiscard]] auto alias() const -> llvm::StringRef { return m_alias; }
    void setAlias(llvm::StringRef alias) { m_alias = alias; }

    [[nodiscard]] auto getDecl() const -> AstDecl* { return m_decl; }
    void setDecl(AstDecl* decl) { m_decl = decl; }

    [[nodiscard]] auto getSymbolTable() const -> SymbolTable* { return m_table; }

    [[nodiscard]] auto identifier() const -> llvm::StringRef {
        if (m_alias.empty()) {
            return m_name;
        }
        return m_alias;
    }

    [[nodiscard]] auto getLlvmLinkage() const -> llvm::GlobalValue::LinkageTypes {
        if (m_flags.external) {
            return llvm::GlobalValue::LinkageTypes::ExternalLinkage;
        }
        return llvm::GlobalValue::LinkageTypes::InternalLinkage;
    }

    // Make vanilla new/delete illegal.
    auto operator new(size_t) -> void* = delete;
    void operator delete(void*) = delete;

    // Allow placement new
    auto operator new(size_t size, void* ptr) -> void* {
        assert(size >= sizeof(Symbol));
        assert(ptr);
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
    ValueFlags m_flags {};
    StateFlags m_state {};
    std::optional<Token::Value> m_constantValue {};
};

} // namespace lbc
