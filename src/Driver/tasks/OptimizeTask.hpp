//
// Created by Albert Varaksin on 15/06/2026.
//
#pragma once
#include "Driver/Task.hpp"

namespace lbc {

/**
 * Optimisation stage: hands the unit's module to the LLVM optimiser (`opt`),
 * shelled out via the toolchain. The module is exchanged as bitcode through
 * temporary files; the optimised result replaces the unit's module.
 */
class OptimizeTask final : public Task {
public:
    explicit OptimizeTask(std::string optimizer)
    : m_optimizer(std::move(optimizer)) {}

    [[nodiscard]] auto run(Context& context, Unit& unit) -> DiagResult<void> override;

private:
    std::string m_optimizer; ///< full path to the opt binary
};

} // namespace lbc
