//
// Created by Albert Varaksin on 15/06/2026.
//
#pragma once
#include "Driver/Task.hpp"

namespace lbc {

/**
 * Backend stage: verify the unit's LLVM module and write it as textual LLVM IR
 * to the unit's output (a file, or stdout when none is set).
 *
 * Native object/assembly emission and linking are handled by separate stages
 * that shell out to the toolchain; only the in-process LLVM IR path lives here.
 */
class EmitTask final : public Task {
public:
    [[nodiscard]] auto run(Context& context, Unit& unit) -> DiagResult<void> override;
};

} // namespace lbc
