//
// Created by Albert Varaksin on 15/06/2026.
//
#include "EmitNativeTask.hpp"
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Program.h>
#include "Driver/Context.hpp"
#include "Driver/Toolchain.hpp"
using namespace lbc;

auto EmitNativeTask::run(Context& context, Artefact input) -> DiagResult<Artefact> {
    const auto& options = context.getOptions();
    auto& diag = context.getDiag();
    const auto fail = [&](const llvm::StringRef reason,
                          const std::source_location& loc = std::source_location::current()) -> DiagError {
        return DiagError { diag.log(diagnostics::codegenFailed(reason.str()), {}, {}, loc) };
    };

    const Toolchain toolchain { context };
    TRY_DECL(codegen, toolchain.getCodeGen())

    // Object or assembly, written to a temporary (an intermediate for linking)
    // or to the build path (the final artifact), per the configured file options.
    const bool assembly = options.getOutputType() == CompileOptions::OutputType::Assembly;
    const llvm::StringRef extension = assembly ? "s" : "o";

    // Own the output up front so a failure below deletes it (if temporary).
    Artefact output { m_option.isTemporary() ? context.createTempFile(extension) : options.artifactPath(m_option.baseName, extension),
                      m_option.isTemporary() };
    if (output.path().empty()) {
        return fail("failed to create output file");
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
    args.push_back(input.path());
    args.push_back("-o");
    args.push_back(output.path());

    std::string error;
    const int code = llvm::sys::ExecuteAndWait(codegen, args, std::nullopt, {}, 0, 0, &error);
    if (code != 0) {
        return fail(error.empty() ? "llc exited with code " + std::to_string(code) : error);
    }

    // The consumed input artefact is deleted here (if temporary) as it goes out of scope.
    return output;
}
