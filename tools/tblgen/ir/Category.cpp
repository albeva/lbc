//
// Created by Albert Varaksin on 28/02/2026.
//
#include "Category.hpp"
#include "IRInstGen.hpp"
using namespace ir;
using namespace llvm;
using namespace std::string_literals;

// -----------------------------------------------------------------------------
// Instructions
// -----------------------------------------------------------------------------

Instruction::Instruction(
    const Record* record,
    const Category* category,
    const IRInstGen& /* gen */
)
: m_record(record)
, m_category(category) {
}

// -----------------------------------------------------------------------------
// Instruction arguments
// -----------------------------------------------------------------------------

Arg::Arg(const Record* record)
: m_record(record)
, m_cpp(record->getValueAsString("cpp"))
, m_name(record->getValueAsString("name"))
, m_vararg(record->getValueAsBit("vararg")) {
}

// -----------------------------------------------------------------------------
// Instruction categories
// -----------------------------------------------------------------------------

Category::Category(const Record* record, const Category* parent, const IRInstGen& gen)
: m_record(record)
, m_parent(parent) {
    // child categories
    for (const Record* klass : gen.getClasses()) {
        if (klass->hasDirectSuperClass(record)) {
            m_children.emplace_back(std::make_unique<Category>(klass, this, gen));
        }
    }

    // owned instructions
    for (const Record* instr : gen.getInstructions()) {
        if (instr->hasDirectSuperClass(record)) {
            m_instructions.emplace_back(std::make_unique<Instruction>(instr, this, gen));
        }
    }
}

auto Category::getName() const -> std::string {
    const auto name = m_record->getName();
    return name.lower();
}

auto Category::getClassName() const -> std::string {
    return (m_record->getName() + "Instruction").str();
}
