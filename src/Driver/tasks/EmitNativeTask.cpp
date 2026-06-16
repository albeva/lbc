//
// Created by Albert Varaksin on 15/06/2026.
//
#include "EmitNativeTask.hpp"
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Program.h>
#include "Driver/Context.hpp"
#include "Driver/Toolchain.hpp"
using namespace lbc;

auto EmitNativeTask::run(Context& context, std::string input) -> DiagResult<std::string> {
    const auto& options = context.getOptions();
    auto& diag = context.getDiag();
    const auto fail = [&](const llvm::StringRef reason,
                          const std::source_location& loc = std::source_location::current()) -> DiagError {
        return DiagError { diag.log(diagnostics::codegenFailed(reason.str()), {}, {}, loc) };
    };

    const Toolchain toolchain { context };
    TRY_DECL(codegen, toolchain.getCodeGen())

    const bool assembly = options.getOutputType() == CompileOptions::OutputType::Assembly;

    // For an executable this object is an intermediate to be linked, so emit it
    // to a temporary; otherwise it is the final artifact at the configured path.
    std::string output;
    if (options.getOutputType() == CompileOptions::OutputType::Executable) {
        output = context.createTempFile("o");
        if (output.empty()) {
            return fail("failed to create temporary file");
        }
    } else {
        output = options.getOutputPath().str();
    }

    // Run: llc -filetype=<asm|obj> [--output-asm-variant=1] -mtriple=<triple> <input.bc> -o <output>
    const std::string mtriple = "-mtriple=" + context.getTriple().str();

    llvm::SmallVector<llvm::StringRef> args;
    args.push_back(codegen);
    args.push_back(assembly ? "-filetype=asm" : "-filetype=obj");
    if (assembly) {
        args.push_back("--output-asm-variant=1"); // Intel syntax for x86; ignored elsewhere
    }
    args.push_back(mtriple);
    args.push_back(input);
    args.push_back("-o");
    args.push_back(output);

    std::string error;
    const int code = llvm::sys::ExecuteAndWait(codegen, args, std::nullopt, {}, 0, 0, &error);
    if (code != 0) {
        return fail(error.empty() ? "llc exited with code " + std::to_string(code) : error);
    }

    return output;
}
