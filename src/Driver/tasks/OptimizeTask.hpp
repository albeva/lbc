//
// Created by Albert Varaksin on 15/06/2026.
//
#pragma once
#include "Driver/Task.hpp"

namespace lbc {

/**
 * Optimisation stage: runs the LLVM optimiser (`opt`), shelled out via the
 * toolchain, over a bitcode file. Takes the input bitcode path and returns the
 * path to a freshly optimised bitcode file. At `-O0` no optimisation is
 * requested, so the input bitcode is passed straight through.
 *
 * The optimiser location and level are read from the context's options.
 */
class OptimizeTask final : public Task<std::string, std::string> {
public:
    using Task::Task;
    [[nodiscard]] auto run(std::string input) -> DiagResult<std::string> override;
};

} // namespace lbc
