//
// Created by Albert Varaksin on 16/06/2026.
//
#include "EmitBinaryTask.hpp"
#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/Program.h>
#include "Driver/Context.hpp"
#include "Driver/Toolchain.hpp"
using namespace lbc;

auto EmitBinaryTask::run(std::vector<std::string> objects) -> DiagResult<std::string> {
    const auto& options = m_context.getOptions();
    auto& diag = m_context.getDiag();
    const auto fail = [&](const llvm::StringRef reason,
                          const std::source_location& loc = std::source_location::current()) -> DiagError {
        return DiagError { diag.log(diagnostics::linkerFailed(reason.str()), {}, {}, loc) };
    };

    const std::string output = options.getOutputPath().str();
    const Toolchain toolchain { m_context };
    TRY_DECL(linker, toolchain.getLinker())

    // Run: cc <objects...> -o <output>
    llvm::SmallVector<llvm::StringRef> args;
    args.push_back(linker);
    for (const auto& object : objects) {
        args.push_back(object);
    }
    args.push_back("-o");
    args.push_back(output);

    std::string error;
    const int code = llvm::sys::ExecuteAndWait(linker, args, std::nullopt, {}, 0, 0, &error);
    if (code != 0) {
        return fail(error.empty() ? "linker exited with code " + std::to_string(code) : error);
    }

    return output;
}
