//
// Created by Albert Varaksin on 16/06/2026.
//
#include "WriteBitcodeTask.hpp"
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include "Driver/Context.hpp"
using namespace lbc;

auto WriteBitcodeTask::run(Context& context, std::unique_ptr<llvm::Module> module) -> DiagResult<std::string> {
    const auto path = context.createTempFile("bc");
    if (path.empty()) {
        return DiagError { context.getDiag().log(diagnostics::cannotOpenOutput("(temporary)", "failed to create temporary file")) };
    }

    std::error_code error;
    llvm::raw_fd_ostream out { path, error };
    if (error) {
        return DiagError { context.getDiag().log(diagnostics::cannotOpenOutput(path, error.message())) };
    }
    llvm::WriteBitcodeToFile(*module, out);

    // The module is dropped on return; the bitcode file is now the carrier of the IR.
    return path;
}
