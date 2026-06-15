//
// Created by Albert Varaksin on 15/06/2026.
//
#pragma once
#include "Driver/Task.hpp"

namespace lbc {

/**
 * Code-generation stage: lowers the unit's module to a native object or
 * assembly file by shelling out to the LLVM code generator (`llc`). The output
 * kind (object vs assembly) and target (arch / pointer width) are read from the
 * options. The module is handed over as bitcode through a temporary file.
 *
 * Assembly is emitted in Intel syntax. For an object, the resulting path is
 * recorded on the unit for the linker.
 */
class CodeGenTask final : public Task {
public:
    explicit CodeGenTask(std::string codegen)
    : m_codegen(std::move(codegen)) {}

    [[nodiscard]] auto run(Context& context, Unit& unit) -> DiagResult<void> override;

private:
    std::string m_codegen; ///< full path to the llc binary
};

} // namespace lbc
