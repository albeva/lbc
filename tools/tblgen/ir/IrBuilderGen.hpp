//
// Created by Albert Varaksin on 01/03/2026.
//
#pragma once
#include "IrGen.hpp"
namespace ir {
using namespace llvm;
/**
 * TableGen backend that reads Instructions.td and emits Builder.hpp.
 * Generates the IR builder interface.
 */
class IrBuilderGen final : public IrGen {
public:
    static constexpr auto genName = "lbc-ir-builder";

    IrBuilderGen(
        raw_ostream& os,
        const RecordKeeper& records
    );

    [[nodiscard]] auto run() -> bool override;
    void builderClass();
    void builders();
    void builder(const IrNodeClass* node);
};
} // namespace ir
