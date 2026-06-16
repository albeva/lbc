//
// Created by Albert Varaksin on 15/06/2026.
//
#pragma once
#include "Driver/Artefact.hpp"
#include "Driver/Task.hpp"

namespace llvm {
class Module;
} // namespace llvm

namespace lbc {

/**
 * Backend stage: verify an LLVM module and write it as textual LLVM IR to the
 * context's resolved output destination (a file, or stdout when empty).
 *
 * Returns the path it wrote (empty when it went to stdout). Native
 * object/assembly emission and linking are handled by separate stages that
 * shell out to the toolchain; only the in-process LLVM IR path lives here.
 */
class EmitLlvmTask final : public Task<std::unique_ptr<llvm::Module>, Artefact> {
public:
    explicit EmitLlvmTask(TaskOption option)
    : m_option(std::move(option)) {}

    [[nodiscard]] auto run(Context& context, std::unique_ptr<llvm::Module> module) -> DiagResult<Artefact> override;

private:
    TaskOption m_option;
};

} // namespace lbc
