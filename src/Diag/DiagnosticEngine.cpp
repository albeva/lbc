//
// Created by Albert on 03/06/2021.
//
#include "DiagnosticEngine.hpp"
#include "Driver/Context.hpp"
using namespace lbc;

namespace {
constexpr std::array messages{
    #define DIAG(ID, STR, ...) STR,
    ALL_ERRORS(DIAG)
    #undef DIAG
};
} // namespace

DiagnosticEngine::DiagnosticEngine(Context& context)
: m_context{ context } {}

auto DiagnosticEngine::getText(Diag diag) -> const char* {
    return messages.at(static_cast<size_t>(diag));
}

auto DiagnosticEngine::getKind(Diag /* diag */) -> llvm::SourceMgr::DiagKind {
    return llvm::SourceMgr::DK_Error;
}

void DiagnosticEngine::print(Diag diag, llvm::SMLoc loc, const std::string& str, llvm::ArrayRef<llvm::SMRange> ranges) {
    if (m_ignoreErrors) {
        return;
    }
    auto kind = getKind(diag);
    m_errorCounter++;
    m_context.getSourceMrg().PrintMessage(loc, kind, str, ranges);
}
