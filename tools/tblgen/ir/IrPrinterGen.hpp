//
// Created by Albert Varaksin on 11/04/2026.
//
#pragma once
#include "IrGen.hpp"
namespace ir {
using namespace llvm;
/**
 * TableGen backend that reads Instructions.td and emits Printer.cpp.
 * Generates the IR printer implementation with per-instruction print methods.
 */
class IrPrinterGen final : public IrGen {
public:
    static constexpr auto genName = "lbc-ir-printer";

    IrPrinterGen(
        raw_ostream& os,
        const RecordKeeper& records
    );

    [[nodiscard]] auto run() -> bool override;
    void preNamespace() override {}

private:
    void dispatch();
    void printInstruction(const IrNodeClass* node);

    [[nodiscard]] static auto hasResult(const lib::TreeNode* node) -> bool;
    [[nodiscard]] static auto emitCall(const lib::TreeNodeArg& arg) -> std::string;
};
} // namespace ir
