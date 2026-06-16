//
// Created by Albert Varaksin on 16/06/2026.
//
#include "EmitBinaryTask.hpp"
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Program.h>
#include "Driver/Context.hpp"
#include "Driver/Toolchain.hpp"
using namespace lbc;

auto EmitBinaryTask::run(Context& context, std::vector<Artefact> objects) -> DiagResult<Artefact> {
    const auto& options = context.getOptions();
    auto& diag = context.getDiag();
    const auto fail = [&](const llvm::StringRef reason,
                          const std::source_location& loc = std::source_location::current()) -> DiagError {
        return DiagError { diag.log(diagnostics::linkerFailed(reason.str()), {}, {}, loc) };
    };

    const Toolchain toolchain { context };
    TRY_DECL(linker, toolchain.getLinker())

    // Own the output up front so a failure below deletes it (if temporary).
    Artefact output { m_option.isTemporary() ? context.createTempFile("") : options.artifactPath(m_option.baseName, ""),
                      m_option.isTemporary() };

    // Run: cc <objects...> -o <output>
    llvm::SmallVector<llvm::StringRef> args;
    args.push_back(linker);
    for (const auto& object : objects) {
        args.push_back(object.path());
    }
    args.push_back("-o");
    args.push_back(output.path());

    std::string error;
    const int code = llvm::sys::ExecuteAndWait(linker, args, std::nullopt, {}, 0, 0, &error);
    if (code != 0) {
        return fail(error.empty() ? "linker exited with code " + std::to_string(code) : error);
    }

    // The objects vector is dropped here, deleting any temporary objects while
    // leaving pre-built (non-temporary) inputs in place.
    return output;
}
