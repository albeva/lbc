//
// Created by Albert Varaksin on 28/02/2026.
//
#include "IRInstGen.hpp"
using namespace ir;
using namespace llvm;
using namespace std::string_literals;

Instruction::Instruction(
    const Record* record,
    const Category* category,
    const IRInstGen& /* gen */
)
: m_record(record)
, m_category(category) {
}

Category::Category(const Record* record, const Category* parent, const IRInstGen& gen)
: m_record(record)
, m_parent(parent) {
    for (const Record* instr : gen.getInstructionRecords()) {
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
