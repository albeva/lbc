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

DiagEngine::~DiagEngine() {
    if (m_autoPrint) {
        print();
    }
}

auto DiagEngine::count(const llvm::SourceMgr::DiagKind kind) const -> std::size_t {
    const auto found = std::ranges::count_if(m_messages, [&](const auto& entry) {
        return entry.kind.getSeverity() == kind;
    });
    return static_cast<std::size_t>(found);
}

auto DiagEngine::hasErrors() const -> bool {
    return std::ranges::any_of(m_messages, [](const auto& entry) static {
        return entry.kind.getSeverity() == llvm::SourceMgr::DK_Error;
    });
}

auto DiagEngine::getKind(const DiagIndex index) const -> DiagKind {
    const auto real = static_cast<std::size_t>(index.get());
    return m_messages.at(real).kind;
}

auto DiagEngine::getDiagnostic(const DiagIndex index) const -> const llvm::SMDiagnostic& {
    const auto real = static_cast<std::size_t>(index.get());
    return m_messages.at(real).diagnostic;
}

auto DiagEngine::getLocation(const DiagIndex index) const -> const std::source_location& {
    const auto real = static_cast<std::size_t>(index.get());
    return m_messages.at(real).location;
}

auto DiagEngine::log(
    const DiagMessage& message,
    const llvm::SMLoc loc,
    const llvm::ArrayRef<llvm::SMRange>& ranges,
    const std::source_location& location
) -> DiagIndex {
    const auto index = m_messages.size();
    auto diagnostic = [&] -> llvm::SMDiagnostic {
        const auto theLoc = [&] -> llvm::SMLoc {
            if (loc.isValid()) {
                return loc;
            }
            if (ranges.size() == 1) {
                return ranges.front().Start;
            }
            return {};
        }();
        if (theLoc.isValid()) {
            return m_context.getSourceMgr().GetMessage(theLoc, message.first.getSeverity(), message.second, ranges);
        }
        return { "", message.first.getSeverity(), message.second };
    }();
    m_messages.emplace_back(message.first, std::move(diagnostic), location);
    return DiagIndex(static_cast<DiagIndex::Value>(index));
}

void DiagEngine::print() const {
    for (const auto& message : m_messages) {
        m_context.getSourceMgr().PrintMessage(llvm::outs(), message.diagnostic, true);
        const auto& loc = m_messages.back().location;
        llvm::outs() << std::format("{}\n", loc);
    }
}
