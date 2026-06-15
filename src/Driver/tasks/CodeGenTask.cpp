//
// Created by Albert Varaksin on 15/06/2026.
//
#include "CodeGenTask.hpp"
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FileUtilities.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Program.h>
#include "Driver/Context.hpp"
using namespace lbc;

namespace {
/** Default output name: the source stem with the given extension, in the CWD. */
auto defaultOutputName(const llvm::StringRef source, const llvm::StringRef extension) -> std::string {
    return (llvm::sys::path::stem(source) + "." + extension).str();
}
} // namespace

auto CodeGenTask::run(Context& context, Unit& unit) -> DiagResult<void> {
    const auto& options = context.getOptions();
    auto& diag = context.getDiag();
    const auto fail = [&](const llvm::StringRef reason) -> DiagError {
        return DiagError { diag.log(diagnostics::codegenFailed(reason.str())) };
    };

    const bool assembly = options.getOutputType() == CompileOptions::OutputType::Assembly;

    // Serialise the module to a temporary bitcode file for llc to consume.
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

    const std::string outputPath = unit.outputPath.empty()
                                     ? defaultOutputName(unit.sourcePath, assembly ? "s" : "o")
                                     : unit.outputPath;

    // Run: llc -filetype=<asm|obj> [--output-asm-variant=1] -mtriple=<triple> <input> -o <output>
    const std::string mtriple = "-mtriple=" + context.getTriple().str();

    llvm::SmallVector<llvm::StringRef> args;
    args.push_back(m_codegen);
    args.push_back(assembly ? "-filetype=asm" : "-filetype=obj");
    if (assembly) {
        args.push_back("--output-asm-variant=1"); // Intel syntax for x86; ignored elsewhere
    }
    args.push_back(mtriple);
    args.push_back(inputPath);
    args.push_back("-o");
    args.push_back(outputPath);

    std::string error;
    const int code = llvm::sys::ExecuteAndWait(m_codegen, args, std::nullopt, {}, 0, 0, &error);
    if (code != 0) {
        return fail(error.empty() ? "llc exited with code " + std::to_string(code) : error);
    }

    // Only an object feeds the linker; assembly is a terminal artifact.
    if (!assembly) {
        unit.objectPath = outputPath;
    }
    return {};
}
