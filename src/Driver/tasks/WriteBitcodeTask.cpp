//
// Created by Albert Varaksin on 16/06/2026.
//
#include "WriteBitcodeTask.hpp"
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include "Driver/Context.hpp"
using namespace lbc;

auto WriteBitcodeTask::run(Context& context, std::unique_ptr<llvm::Module> module) -> DiagResult<Artefact> {
    // Own the output up front so a failure below deletes it (if temporary).
    Artefact output { m_option.isTemporary() ? context.createTempFile("bc") : context.getOptions().artifactPath(m_option.baseName, "bc"),
                      m_option.isTemporary() };
    if (output.path().empty()) {
        return DiagError { context.getDiag().log(diagnostics::cannotOpenOutput("(bitcode)", "failed to create output file")) };
    }

    std::error_code error;
    llvm::raw_fd_ostream out { output.path(), error };
    if (error) {
        return DiagError { context.getDiag().log(diagnostics::cannotOpenOutput(output.path().str(), error.message())) };
    }
    llvm::WriteBitcodeToFile(*module, out);

    // The module is dropped on return; the bitcode artefact now carries the IR.
    return output;
}
