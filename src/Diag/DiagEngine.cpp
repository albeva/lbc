//
// Created by Albert Varaksin on 16/02/2026.
//
#include "DiagEngine.hpp"

#include <llvm/Support/raw_ostream.h>

#include "Driver/Context.hpp"
using namespace lbc;

DiagEngine::DiagEngine(Context& context)
: m_context(context) {
}

DiagEngine::~DiagEngine() = default;

auto DiagEngine::count(llvm::SourceMgr::DiagKind kind) const -> std::size_t {
    const auto found = std::ranges::count_if(m_messages, [&](const auto& entry) {
        return entry.getKind() == kind;
    });
    return static_cast<std::size_t>(found);
}

auto DiagEngine::hasErrors() const -> bool {
    return std::ranges::any_of(m_messages, [](const auto& entry) static {
        return entry.getKind() == llvm::SourceMgr::DiagKind::DK_Error;
    });
}

auto DiagEngine::get(const DiagIndex index) const -> const llvm::SMDiagnostic& {
    const auto real = static_cast<std::size_t>(index.get());
    return m_messages.at(real);
}

auto DiagEngine::log(
    llvm::SMLoc loc,
    llvm::SourceMgr::DiagKind kind,
    const std::string& msg,
    llvm::ArrayRef<llvm::SMRange> ranges
) -> DiagIndex {
    const auto index = m_messages.size();
    m_messages.emplace_back(m_context.getSourceMgr().GetMessage(loc, kind, msg, ranges));
    return DiagIndex(static_cast<DiagIndex::Value>(index));
}

void DiagEngine::print() const {
    for (const auto& message : m_messages) {
        m_context.getSourceMgr().PrintMessage(llvm::outs(), message, true);
    }
}
