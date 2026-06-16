//
// Created by Albert Varaksin on 15/06/2026.
//
#include "EmitLlvmTask.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include "Driver/Context.hpp"
using namespace lbc;

auto EmitLlvmTask::run(Context& context, std::unique_ptr<llvm::Module> module) -> DiagResult<Artefact> {
    if (llvm::verifyModule(*module, &llvm::errs())) {
        return DiagError { context.getDiag().log(diagnostics::backendVerificationFailed()) };
    }

    // Own the output up front so a failure below deletes it (if temporary).
    Artefact output { m_option.isTemporary() ? context.createTempFile("ll") : context.getOptions().artifactPath(m_option.baseName, "ll"),
                      m_option.isTemporary() };
    if (output.path().empty()) {
        return DiagError { context.getDiag().log(diagnostics::cannotOpenOutput("(llvm ir)", "failed to create output file")) };
    }

    std::error_code error;
    llvm::raw_fd_ostream out { output.path(), error };
    if (error) {
        return DiagError { context.getDiag().log(diagnostics::cannotOpenOutput(output.path().str(), error.message())) };
    }
    module->print(out, nullptr);

    return output;
}
