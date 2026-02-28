//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include <llvm/Support/raw_ostream.h>
#include <llvm/TableGen/Record.h>
#include "GeneratorBase.hpp"
namespace ir {
class IRInstGen;
class Category;

/// represent an instruction
class Instruction final {
public:
    Instruction(const Record* record, const Category* category, const IRInstGen& gen);

    [[nodiscard]] auto getName() const -> StringRef { return m_record->getName(); }
    [[nodiscard]] auto getRecord() const -> const Record* { return m_record; }
    [[nodiscard]] auto getCategory() const -> const Category* { return m_category; }

private:
    const Record* m_record;
    const Category* m_category;
};

/// represent instructions category
class Category final {
public:
    Category(const Record* record, const Category* parent, const IRInstGen& gen);

    [[nodiscard]] auto getName() const -> std::string;
    [[nodiscard]] auto getClassName() const -> std::string;
    [[nodiscard]] auto getRecord() const -> const Record* { return m_record; }
    [[nodiscard]] auto getInstructions() const -> const std::vector<std::unique_ptr<Instruction>>& { return m_instructions; }
    [[nodiscard]] auto getParent() const -> const Category* { return m_parent; }
    [[nodiscard]] auto getChildren() const -> const std::vector<Category*>& { return m_children; }
    [[nodiscard]] auto getChildren() -> std::vector<Category*>& { return m_children; }

private:
    const Record* m_record;
    const Category* m_parent;
    std::vector<std::unique_ptr<Instruction>> m_instructions;
    std::vector<Category*> m_children;
};

/** TableGen backend that reads Instructions.td and emits Instructions.hpp. */
class IRInstGen final : public GeneratorBase {
public:
    static constexpr auto genName = "lbc-ir-inst-def";

    IRInstGen(raw_ostream& os, const RecordKeeper& records);

    [[nodiscard]] auto run() -> bool override;

    [[nodiscard]] auto getInstrClass() const -> const Record* { return m_instrClass; }
    [[nodiscard]] auto getCategoryClasses() const -> const std::vector<const Record*>& { return m_categoryClasses; }
    [[nodiscard]] auto getCategories() const -> const std::vector<std::unique_ptr<Category>>& { return m_categories; }
    [[nodiscard]] auto getInstructionRecords() const -> const std::vector<const Record*>& { return m_instructionRecords; }

private:
    void kindsEnum();
    void instructionClass();
    void categoryClass(const Category* category);

    const Record* m_instrClass;
    std::vector<const Record*> m_instructionRecords;
    std::vector<const Record*> m_categoryClasses;
    std::vector<std::unique_ptr<Category>> m_categories;
};
} // namespace ir
