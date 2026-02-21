//
// Created by Albert Varaksin on 21/02/2026.
//
#pragma once
#include <llvm/TableGen/Record.h>
using namespace llvm;

class Type;
class TypeBaseGen;

/**
 * Represent type category, which is a collection of grouped types
 */
class TypeCategory final {
public:
    explicit TypeCategory(const Record* record, const TypeBaseGen& gen);

    [[nodiscard]] auto getRecord() const -> const Record* { return m_record; }
    [[nodiscard]] auto getTypes() const -> const std::vector<std::unique_ptr<Type>>& { return m_types; }
    [[nodiscard]] auto isSingle() const -> bool;

private:
    const Record* m_record;
    std::vector<std::unique_ptr<Type>> m_types;
};

/**
 * Represent a single type
 */
class Type final {
public:
    Type(const Record* record, const TypeCategory* category);

    [[nodiscard]] auto getRecord() const -> const Record* { return m_record; }
    [[nodiscard]] auto getCategory() const -> const TypeCategory* { return m_category; }
    [[nodiscard]] auto getEnumName() const -> llvm::StringRef { return m_enumName; }
    [[nodiscard]] auto getBackingClassName() const -> std::optional<llvm::StringRef>;

private:
    const Record* m_record;
    const TypeCategory* m_category;
    llvm::StringRef m_enumName;
};
