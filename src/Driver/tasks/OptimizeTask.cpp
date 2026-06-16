//
// Created by Albert Varaksin on 15/06/2026.
//
#include "OptimizeTask.hpp"
#include <llvm/Support/Program.h>
#include "Driver/Context.hpp"
#include "Driver/Toolchain.hpp"
using namespace lbc;

auto OptimizeTask::run(Context& context, Artefact input) -> DiagResult<Artefact> {
    const auto& options = context.getOptions();

    // -O0 requests no optimisation: hand the bitcode straight through.
    if (options.getOptimizationLevel() == CompileOptions::OptimizationLevel::O0) {
        return input;
    }

    auto& diag = context.getDiag();
    const auto fail = [&](const llvm::StringRef reason,
                          const std::source_location& loc = std::source_location::current()) -> DiagError {
        return DiagError { diag.log(diagnostics::optimizerFailed(reason.str()), {}, {}, loc) };
    };

    const Toolchain toolchain { context };
    TRY_DECL(optimizer, toolchain.getOptimizer())

    // Own the output up front so a failure below deletes it (if temporary).
    Artefact output { m_option.isTemporary() ? context.createTempFile("bc") : options.artifactPath(m_option.baseName, "bc"),
                      m_option.isTemporary() };
    if (output.path().empty()) {
        return fail("failed to create output file");
    }

    // opt -O<level> <input.bc> -o <output.bc>
    const std::array<llvm::StringRef, 5> args { optimizer, options.getOptimizationFlag(), input.path(), "-o", output.path() };
    std::string error;
    const int code = llvm::sys::ExecuteAndWait(optimizer, args, std::nullopt, {}, 0, 0, &error);
    if (code != 0) {
        return fail(error.empty() ? "opt exited with code " + std::to_string(code) : error);
    }

    // The consumed input artefact is deleted here (if temporary) as it goes out of scope.
    return output;
}
