//
// Created by Albert Varaksin on 15/06/2026.
//
#include "EmitTask.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include "Driver/Context.hpp"
using namespace lbc;

auto EmitTask::run(Context& context, Unit& unit) -> DiagResult<void> {
    auto& module = *unit.module;

    if (llvm::verifyModule(module, &llvm::errs())) {
        return DiagError { context.getDiag().log(diagnostics::backendVerificationFailed()) };
    }

    if (unit.outputPath.empty()) {
        module.print(llvm::outs(), nullptr);
        return {};
    }

    std::error_code error;
    llvm::raw_fd_ostream out { unit.outputPath, error };
    if (error) {
        return DiagError { context.getDiag().log(diagnostics::cannotOpenOutput(unit.outputPath, error.message())) };
    }
    module.print(out, nullptr);

    return {};
}
