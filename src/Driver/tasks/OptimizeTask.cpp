//
// Created by Albert Varaksin on 15/06/2026.
//
#include "OptimizeTask.hpp"
#include <llvm/ADT/SmallString.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Program.h>
#include "Driver/Context.hpp"
using namespace lbc;

auto OptimizeTask::run(Context& context, Unit& unit) -> DiagResult<void> {
    auto& diag = context.getDiag();
    const auto fail = [&](const llvm::StringRef reason) -> DiagError {
        return DiagError { diag.log(diagnostics::optimizerFailed(reason.str())) };
    };

    // Serialise the module to a temporary bitcode file.
    llvm::SmallString<128> inputPath;
    if (llvm::sys::fs::createTemporaryFile("lbc", "bc", inputPath)) {
        return fail("cannot create temporary file");
    }
    const llvm::FileRemover inputRemover { inputPath };
    {
        std::error_code error;
        llvm::raw_fd_ostream out { inputPath, error };
        if (error) {
            return fail(error.message());
        }
        llvm::WriteBitcodeToFile(*unit.module, out);
    }

    // Reserve a temporary path for opt's output.
    llvm::SmallString<128> outputPath;
    if (llvm::sys::fs::createTemporaryFile("lbc", "bc", outputPath)) {
        return fail("cannot create temporary file");
    }
    const llvm::FileRemover outputRemover { outputPath };

    // Run: opt -O<level> <input> -o <output>
    const auto optimizationFlag = context.getOptions().getOptimizationFlag();
    const std::array<llvm::StringRef, 5> args { m_optimizer, optimizationFlag, inputPath, "-o", outputPath };
    std::string error;
    const int code = llvm::sys::ExecuteAndWait(m_optimizer, args, std::nullopt, {}, 0, 0, &error);
    if (code != 0) {
        return fail(error.empty() ? "opt exited with code " + std::to_string(code) : error);
    }

    // Parse the optimised bitcode back into the unit's module.
    auto buffer = llvm::MemoryBuffer::getFile(outputPath);
    if (!buffer) {
        return fail(buffer.getError().message());
    }
    auto parsed = llvm::parseBitcodeFile((*buffer)->getMemBufferRef(), unit.llvmContext);
    if (!parsed) {
        return fail(llvm::toString(parsed.takeError()));
    }
    unit.module = std::move(*parsed);

    return {};
}
