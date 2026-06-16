//
// Created by Albert Varaksin on 15/06/2026.
//
#include "Toolchain.hpp"
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Program.h>
#include "Context.hpp"
using namespace lbc;

auto Toolchain::getLinker() const -> DiagResult<std::string> {
    // The linker driver is the host C compiler — it knows the system libraries
    // and startup files — so it is resolved on PATH, not in the LLVM toolchain
    // directory (which only supplies opt/llc).
    if (auto found = llvm::sys::findProgramByName("cc")) {
        return *found;
    }
    return DiagError { m_context.getDiag().log(diagnostics::toolNotFound("cc")) };
}

auto Toolchain::resolve(const llvm::StringRef tool) const -> DiagResult<std::string> {
    const llvm::StringRef dir = m_context.getOptions().getToolchainPath();

    // No configured directory: look the tool up on PATH (this also applies the
    // platform executable suffix).
    if (dir.empty()) {
        if (auto found = llvm::sys::findProgramByName(tool)) {
            return *found;
        }
    } else {
        llvm::SmallString<256> full { dir };
        llvm::sys::path::append(full, tool);
#ifdef _WIN32
        full += ".exe";
#endif
        if (llvm::sys::fs::exists(full)) {
            return std::string { full.data(), full.size() };
        }
    }

    return DiagError { m_context.getDiag().log(diagnostics::toolNotFound(tool.str())) };
}
