//
// Created by Albert Varaksin on 15/06/2026.
//
#pragma once
#include "Driver/Artefact.hpp"
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
class OptimizeTask final : public Task<Artefact, Artefact> {
public:
    explicit OptimizeTask(TaskOption option)
    : m_option(std::move(option)) {}

    [[nodiscard]] auto run(Context& context, Artefact input) -> DiagResult<Artefact> override;

private:
    TaskOption m_option;
};

} // namespace lbc
