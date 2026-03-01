// Custom TableGen backend for generating IR instruction definitions.
// Reads Instructions.td and emits Instructions.hpp
#include "IRInstGen.hpp"
#include "Category.hpp"
using namespace ir;

IRInstGen::IRInstGen(
    raw_ostream& os,
    const RecordKeeper& records
)
: GeneratorBase(os, records, genName, "lbc::ir", { "pch.hpp", "IR/Instruction.hpp" })
, m_instrClass(records.getClass("Instruction"))
, m_instructions(sortedByDef(records.getAllDerivedDefinitions("Instruction"))) {
    // Get all the classes
    for (const auto& klass : records.getClasses() | std::views::values) {
        m_classes.emplace_back(klass.get());
    }
    std::ranges::sort(m_classes, [](const Record* one, const Record* two) static {
        return one->getID() < two->getID();
    });

    // define categories
    for (const Record* klass : m_classes) {
        if (klass->hasDirectSuperClass(m_instrClass)) {
            m_categories.emplace_back(std::make_unique<Category>(klass, nullptr, *this));
        }
    }
}

IRInstGen::~IRInstGen() = default;

auto IRInstGen::run() -> bool {
    kindsEnum();
    instructionClass();
    categoryClasses();
    return false;
}

void IRInstGen::kindsEnum() {
    doc("Instructions");
    block("enum class InstrKind : std::uint8_t", true, [&] {
        visit([&](const Instruction* instr) {
            line(instr->getName(), ",");
        });
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

void IRInstGen::categoryClasses() {
    visit([&](const Category* category) {
        newline();
        categoryClass(category);
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
