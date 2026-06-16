//
// Created by Albert Varaksin on 15/06/2026.
//
#include "EmitLlvmTask.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include "Driver/Context.hpp"
using namespace lbc;

auto EmitLlvmTask::run(Context& context, std::unique_ptr<llvm::Module> module) -> DiagResult<std::string> {
    if (llvm::verifyModule(*module, &llvm::errs())) {
        return DiagError { context.getDiag().log(diagnostics::backendVerificationFailed()) };
    }

    const std::string output = context.getOptions().getOutputPath().str();
    if (output.empty()) {
        module->print(llvm::outs(), nullptr);
        return output; // empty: went to stdout, nothing to thread onward
    }

    std::error_code error;
    llvm::raw_fd_ostream out { output, error };
    if (error) {
        return DiagError { context.getDiag().log(diagnostics::cannotOpenOutput(output, error.message())) };
    }
    module->print(out, nullptr);

    return output;
}
