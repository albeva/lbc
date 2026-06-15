//
// Created by Albert Varaksin on 15/06/2026.
//
#pragma once
#include "Driver/Task.hpp"

namespace lbc {

/**
 * Frontend stage: lex, parse, analyse, and lower one source file to an
 * in-memory LLVM module. Runs entirely in process. Honours the AST and lbc-IR
 * debug dumps.
 */
class CompileTask final : public Task {
public:
    [[nodiscard]] auto run(Context& context, Unit& unit) -> DiagResult<void> override;
};

} // namespace lbc
