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
 * Frontend stage: lex, parse, analyse, and lower one source file (given by its
 * path) to an in-memory LLVM module. Runs entirely in process. Honours the AST
 * and lbc-IR debug dumps.
 */
class CompileTask final : public Task<std::string, std::unique_ptr<llvm::Module>> {
public:
    using Task::Task;
    [[nodiscard]] auto run(std::string source) -> DiagResult<std::unique_ptr<llvm::Module>> override;
};

} // namespace lbc
