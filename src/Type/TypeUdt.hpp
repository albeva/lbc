//
// Created by Albert on 29/05/2021.
//
#pragma once
#include "pch.hpp"
#include "Type.hpp"

namespace lbc {
class Symbol;
class SymbolTable;
class Context;

/**
 * User defined type
 */
class TypeUDT final : public TypeRoot {
    TypeUDT(Symbol& symbol, SymbolTable& symbolTable, bool packed);
    friend class Context;

public:
    static auto get(Context& context, Symbol& symbol, SymbolTable& symbolTable, bool packed) -> const TypeUDT*;

    constexpr static auto classof(const TypeRoot* type) -> bool {
        return type->getFamily() == TypeFamily::UDT;
    }

    [[nodiscard]] auto asString() const -> std::string final;

    [[nodiscard]] auto getSymbol() const -> Symbol& { return m_symbol; }
    [[nodiscard]] auto getSymbolTable() const -> SymbolTable& { return m_symbolTable; }

protected:
    auto genLlvmType(Context& context) const -> llvm::Type* final;

private:
    Symbol& m_symbol;
    SymbolTable& m_symbolTable;
    bool m_packed;
};

} // namespace lbc
