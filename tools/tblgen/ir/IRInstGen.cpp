// Custom TableGen backend for generating IR instruction definitions.
// Reads Instructions.td and emits Instructions.hpp
#include "IRInstGen.hpp"
using namespace ir;

IRInstGen::IRInstGen(
    raw_ostream& os,
    const RecordKeeper& records
)
: GeneratorBase(os, records, genName, "lbc::ir", { "pch.hpp", "IR/Instruction.hpp" })
, m_instrClass(records.getClass("Instruction"))
, m_instructionRecords(sortedByDef(records.getAllDerivedDefinitions("Instruction"))) {
    // construct categories
    for (const auto& val : records.getClasses() | std::views::values) {
        if (std::ranges::contains(val->getSuperClasses(), m_instrClass)) {
            m_categoryClasses.emplace_back(val.get());
        }
    }
    m_categoryClasses = sortedByDef(m_categoryClasses);
    m_categories.reserve(m_categoryClasses.size());

    const auto findParent = [&](const Record* record) -> Category* {
        const auto klasses = record->getSuperClasses();
        if (klasses.empty()) {
            return nullptr;
        }
        for (const auto& cat : m_categories) {
            if (cat->getRecord() == klasses.back()) {
                return cat.get();
            }
        }
        return nullptr;
    };

    for (const auto* klass : m_categoryClasses) {
        auto* parent = findParent(klass);
        auto& res = m_categories.emplace_back(std::make_unique<Category>(klass, parent, *this));
        if (parent != nullptr) {
            parent->getChildren().emplace_back(res.get());
        }
    }
}

auto IRInstGen::run() -> bool {
    kindsEnum();
    instructionClass();
    for (const auto& cat : m_categories) {
        newline();
        categoryClass(cat.get());
    }
    return false;
}

void IRInstGen::kindsEnum() {
    doc("Instructions");
    block("enum class InstrKind : std::uint8_t", true, [&] {
        for (const auto& cat : m_categories) {
            for (const auto& instr : cat->getInstructions()) {
                line(instr->getName(), ",");
            }
        }
    });
}

void IRInstGen::instructionClass() {
    doc("base class for instructions");
    block("class Instruction : public llvm::ilist_node<Instruction>", true, [&] {
        scope(Scope::Public, true);
        line("NO_COPY_AND_MOVE(Instruction)");
        newline();

        getter("kind", "InstrKind");
        newline();

        classof("Instruction", "getKind", "InstrKind");
        newline();

        scope(Scope::Protected);
        line("explicit constexpr Instruction(const InstrKind kind)", "");
        line(": m_kind(kind) {}", "");
        newline();

        scope(Scope::Private);
        line("InstrKind m_kind");
    });
}

void IRInstGen::categoryClass(const Category* category) {
    doc(category->getName() + " instructions");

    const std::string super = category->getParent() == nullptr ? "Instruction" : category->getParent()->getClassName();
    const auto* fnl = category->getChildren().empty() ? " final" : "";

    block("class [[nodiscard]] " + category->getClassName() + fnl + " : " + super, true, [&] {
        scope(Scope::Public, true);
    });
}
