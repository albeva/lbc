//
// Created by Albert Varaksin on 21/02/2026.
//
#pragma once
#include <llvm/TableGen/Record.h>
using namespace llvm;

class Type;
class TypeBaseGen;

/**
 * Represents a type category (e.g. Sentinel, Primitive, SignedIntegral),
 * which is a collection of grouped types sharing a common TypeKind.
 */
class TypeCategory final {
public:
    explicit TypeCategory(const Record* record, const TypeBaseGen& gen);

    /** Get the underlying TableGen record for this category. */
    [[nodiscard]] auto getRecord() const -> const Record* { return m_record; }

    /** Get the types belonging to this category. */
    [[nodiscard]] auto getTypes() const -> const std::vector<std::unique_ptr<Type>>& { return m_types; }

    /** Check if types in this category are singletons (one instance each). */
    [[nodiscard]] auto isSingle() const -> bool;

private:
    /// The TableGen record defining this category
    const Record* m_record;
    /// Types belonging to this category
    std::vector<std::unique_ptr<Type>> m_types;
};

/**
 * Represents a single type definition from Types.td.
 *
 * Wraps a TableGen record and provides accessors for the type's
 * enum name and optional backing C++ class name.
 */
class Type final {
public:
    Type(const Record* record, const TypeCategory* category);

    /** Get the underlying TableGen record. */
    [[nodiscard]] auto getRecord() const -> const Record* { return m_record; }

    /** Get the category this type belongs to. */
    [[nodiscard]] auto getCategory() const -> const TypeCategory* { return m_category; }

    /** Get the enum name (e.g. "Void", "Integer", "Pointer"). */
    [[nodiscard]] auto getEnumName() const -> llvm::StringRef { return m_enumName; }

    /** Get the backing C++ class name, if specified (e.g. "TypeIntegral"). */
    [[nodiscard]] auto getBackingClassName() const -> std::optional<llvm::StringRef>;

private:
    /// The TableGen record defining this type
    const Record* m_record;
    /// The owning category
    const TypeCategory* m_category;
    /// Enum name derived from the record name (with "Type" suffix stripped)
    llvm::StringRef m_enumName;
};
