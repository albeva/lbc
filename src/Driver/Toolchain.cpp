//
// Created by Albert Varaksin on 15/06/2026.
//
#include "Toolchain.hpp"
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Program.h>
using namespace lbc;

auto Toolchain::resolve(const llvm::StringRef tool) const -> std::string {
    // No configured directory: look the tool up on PATH (this also applies the
    // platform executable suffix).
    if (m_path.empty()) {
        if (auto found = llvm::sys::findProgramByName(tool)) {
            return *found;
        }
        return tool.str();
    }

    llvm::SmallString<256> full { m_path };
    llvm::sys::path::append(full, tool);
#ifdef _WIN32
    full += ".exe";
#endif
    return std::string { full.data(), full.size() };
}
