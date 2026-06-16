//
// Created by Albert Varaksin on 16/06/2026.
//
#pragma once
#include "Driver/Task.hpp"

namespace llvm {
class Module;
} // namespace llvm

namespace lbc {

/**
 * Serialise an in-memory module to a temporary bitcode file, the form the
 * shell-out stages (optimiser, code generator) consume. Done once: the returned
 * bitcode path then flows through the remaining stages. The module is consumed.
 */
class WriteBitcodeTask final : public Task<std::unique_ptr<llvm::Module>, std::string> {
public:
    [[nodiscard]] auto run(Context& context, std::unique_ptr<llvm::Module> module) -> DiagResult<std::string> override;
};

} // namespace lbc
