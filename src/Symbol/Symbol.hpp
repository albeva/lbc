//
// Created by Albert Varaksin on 20/02/2026.
//
#pragma once
#include "pch.hpp"
#include "LiteralValue.hpp"
#include "SymbolTable.hpp"
namespace lbc {
class Type;

/** Visibility of a symbol across module boundaries. */
enum class SymbolVisibility : std::uint8_t {
    Private,
    External
};

/** Bitmask flags describing a symbol's lifecycle state and kind. */
enum class SymbolFlags : std::uint8_t {
    None = 0,
    BeingDefined = 1U << 0U, ///< Symbol is being defined, this is used to prevent circular dependencies
    Defined = 1U << 1U,      ///< Symbol has been defined, but may not yet be usable
    Declared = 1U << 2U,     ///< Symbol has been fully declared and can be used in expressions
    Function = 1U << 3U,     ///< Symbol is a function
    Variable = 1U << 4U,     ///< The symbol is a variable
    Constant = 1U << 5U,     ///< The symbol is a constant
    Type = 1U << 6U,         ///< The symbol is a type
};

/**
 * Represents a named entity in the program (variable, function, constant, or type).
 *
 * Symbols are arena-allocated and owned by the Context. Each symbol tracks its name,
 * type, source location, visibility, and lifecycle state via SymbolFlags.
 */
class Symbol final : public TypedFlags<SymbolFlags> {
public:
    NO_COPY_AND_MOVE(Symbol)

    /** Construct a symbol with the given name, type, and source location. */
    Symbol(llvm::StringRef name, const Type* type, llvm::SMRange origin);

    /** Get the effective name, preferring alias over the original name. */
    [[nodiscard]] auto getSymbolName() const -> llvm::StringRef {
        return m_alias.empty() ? m_name : m_alias;
    }

    /** Get the original declared name. */
    [[nodiscard]] auto getName() const -> llvm::StringRef { return m_name; }
    void setName(const llvm::StringRef name) { m_name = name; }

    /** Get the optional alias for this symbol. */
    [[nodiscard]] auto getAlias() const -> llvm::StringRef { return m_alias; }
    void setAlias(const llvm::StringRef alias) { m_alias = alias; }

    /** Get the type associated with this symbol. */
    [[nodiscard]] auto getType() const -> const Type* { return m_type; }
    void setType(const Type* type) { m_type = type; }

    /** Get the source location where this symbol was declared. */
    [[nodiscard]] auto getRange() const -> llvm::SMRange { return m_range; }
    void setRange(const llvm::SMRange origin) { m_range = origin; }

    /** Get the visibility of this symbol. */
    [[nodiscard]] auto getVisibility() const -> SymbolVisibility { return m_visibility; }
    void setVisibility(const SymbolVisibility visibility) { m_visibility = visibility; }

    /** Check whether this symbol has an associated constant value. */
    [[nodiscard]] auto hasValue() const -> bool { return m_value.has_value(); }
    /** Get the optional constant value associated with this symbol. */
    [[nodiscard]] auto getValue() const -> std::optional<LiteralValue> { return m_value; }
    void setValue(const std::optional<LiteralValue>& constant) { m_value = constant; }

    [[nodiscard]] auto getRelatedSymbols() const -> std::span<Symbol*> { return m_relatedSymbols; }
    void setRelatedSymbols(const std::span<Symbol*> relatedSymbols) { m_relatedSymbols = relatedSymbols; }

private:
    llvm::StringRef m_name;              ///< symbol name
    llvm::StringRef m_alias;             ///< optional alias
    const Type* m_type;                  ///< symbol type
    llvm::SMRange m_range;               ///< declaration location
    SymbolVisibility m_visibility;       ///< visibility of the symbol
    std::optional<LiteralValue> m_value; ///< constant value associated with the symbol
    std::span<Symbol*> m_relatedSymbols; ///< related symbols, e.g. function parameters, or UDT members
};

/** Symbol table for the frontend, mapping names to Symbols. */
class SymbolTable final : public SymbolTableBase<Symbol> {
public:
    using SymbolTableBase::SymbolTableBase;
};

} // namespace lbc
