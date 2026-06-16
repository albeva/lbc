//
// Created by Albert Varaksin on 16/06/2026.
//
#pragma once
#include "Driver/Artefact.hpp"
#include "Driver/Task.hpp"

namespace llvm {
class Module;
} // namespace llvm

namespace lbc {

/**
 * Serialise an in-memory module to a bitcode artefact, the form the shell-out
 * stages (optimiser, code generator) consume. The returned artefact then flows
 * through the remaining stages. The module is consumed.
 */
class WriteBitcodeTask final : public Task<std::unique_ptr<llvm::Module>, Artefact> {
public:
    explicit WriteBitcodeTask(TaskOption option)
    : m_option(std::move(option)) {}

    [[nodiscard]] auto run(Context& context, std::unique_ptr<llvm::Module> module) -> DiagResult<Artefact> override;

private:
    TaskOption m_option;
};

} // namespace lbc
