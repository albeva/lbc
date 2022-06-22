//
// Created by Albert on 03/06/2021.
//
#include "DiagnosticEngine.hpp"
#include "Driver/Context.hpp"
using namespace lbc;

namespace {
constexpr std::array messages{
#define DIAG(LEVEL, ID, STR) STR,
#include "Diagnostics.def.hpp"
};

constexpr std::array diagKind{
#define DIAG(LEVEL, ...) llvm::SourceMgr::DiagKind::DK_##LEVEL,
#include "Diagnostics.def.hpp"
};
} // namespace

DiagnosticEngine::DiagnosticEngine(Context& context) noexcept
: m_context{ context },
  m_sourceMgr{ context.getSourceMrg() } {}

const char* DiagnosticEngine::getText(Diag diag) noexcept {
    return messages[static_cast<size_t>(diag)]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
}

llvm::SourceMgr::DiagKind DiagnosticEngine::getKind(Diag diag) noexcept {
    return diagKind[static_cast<size_t>(diag)]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
}

void DiagnosticEngine::print(Diag diag, llvm::SMLoc loc, const std::string& str, llvm::ArrayRef<llvm::SMRange> ranges) noexcept {
    auto kind = getKind(diag);
    if (kind == llvm::SourceMgr::DK_Error) {
        m_errorCounter++;
    }
    m_sourceMgr.PrintMessage(loc, kind, str, ranges);
}
