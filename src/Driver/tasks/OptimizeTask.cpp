//
// Created by Albert Varaksin on 15/06/2026.
//
#include "OptimizeTask.hpp"
#include <llvm/Support/Program.h>
#include "Driver/Context.hpp"
#include "Driver/Toolchain.hpp"
using namespace lbc;

auto OptimizeTask::run(Context& context, std::string input) -> DiagResult<std::string> {
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

    const auto output = context.createTempFile("bc");
    if (output.empty()) {
        return fail("failed to create temporary file");
    }

    const Toolchain toolchain { context };
    TRY_DECL(optimizer, toolchain.getOptimizer())

    // opt -O<level> <input.bc> -o <output.bc>
    const std::array<llvm::StringRef, 5> args { optimizer, options.getOptimizationFlag(), input, "-o", output };
    std::string error;
    const int code = llvm::sys::ExecuteAndWait(optimizer, args, std::nullopt, {}, 0, 0, &error);
    if (code != 0) {
        return fail(error.empty() ? "opt exited with code " + std::to_string(code) : error);
    }

    return output;
}
