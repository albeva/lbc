//
// Created by Albert Varaksin on 28/02/2026.
//
#pragma once
#include <llvm/Support/raw_ostream.h>
#include <llvm/TableGen/Record.h>
#include "GeneratorBase.hpp"

/** TableGen backend that reads Instructions.td and emits Instructions.hpp. */
class IRInstGen final : public GeneratorBase {
public:
    static constexpr auto genName = "lbc-ir-inst-def";

    IRInstGen(raw_ostream& os, const RecordKeeper& records);

    [[nodiscard]] auto run() -> bool override;
};
