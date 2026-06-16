//
// Created by Albert Varaksin on 15/06/2026.
//
#pragma once
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
class EmitLlvmTask final : public Task<std::unique_ptr<llvm::Module>, std::string> {
public:
    using Task::Task;
    [[nodiscard]] auto run(std::unique_ptr<llvm::Module> module) -> DiagResult<std::string> override;
};

} // namespace lbc
