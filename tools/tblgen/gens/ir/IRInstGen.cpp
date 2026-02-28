// Custom TableGen backend for generating IR instruction definitions.
// Reads Instructions.td and emits Instructions.hpp
#include "IRInstGen.hpp"

IRInstGen::IRInstGen(
    raw_ostream& os,
    const RecordKeeper& records
)
: GeneratorBase(os, records, genName, "lbc::ir", { "pch.hpp", "IR/Instruction.hpp" }) {
}

auto IRInstGen::run() -> bool {
    return false;
}
