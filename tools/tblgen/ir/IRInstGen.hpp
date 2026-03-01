//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include <llvm/Support/raw_ostream.h>
#include <llvm/TableGen/Record.h>
#include "Category.hpp"
#include "GeneratorBase.hpp"
namespace ir {

/** TableGen backend that reads Instructions.td and emits Instructions.hpp. */
class IRInstGen final : public GeneratorBase {
public:
    static constexpr auto genName = "lbc-ir-inst-def";

    IRInstGen(raw_ostream& os, const RecordKeeper& records);
    ~IRInstGen() override;

    [[nodiscard]] auto run() -> bool override;

    [[nodiscard]] auto getInstrClass() const -> const Record* { return m_instrClass; }
    [[nodiscard]] auto getClasses() const -> const auto& { return m_classes; }
    [[nodiscard]] auto getCategories() const -> const auto& { return m_categories; }
    [[nodiscard]] auto getInstructions() const -> const auto& { return m_instructions; }

    template<std::invocable<const Category*> Func>
    void visit(Func&& func) {
        for (const auto& category : m_categories) {
            category->visit(std::forward<Func>(func));
        }
    }

    template<std::invocable<const Instruction*> Func>
    void visit(Func&& func) {
        for (const auto& category : m_categories) {
            category->visit(std::forward<Func>(func));
        }
    }

private:
    void kindsEnum();
    void instructionClass();
    void categoryClasses();
    void categoryClass(const Category* category);

    const Record* m_instrClass;
    std::vector<const Record*> m_instructions;
    std::vector<const Record*> m_classes;
    std::vector<std::unique_ptr<Category>> m_categories;
};
} // namespace ir
