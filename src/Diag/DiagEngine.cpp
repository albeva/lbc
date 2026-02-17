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
        return entry.kind.getSeverity() == kind;
    });
    return static_cast<std::size_t>(found);
}

auto DiagEngine::hasErrors() const -> bool {
    return std::ranges::any_of(m_messages, [](const auto& entry) static {
        return entry.kind.getSeverity() == llvm::SourceMgr::DK_Error;
    });
}

auto DiagEngine::getKind(DiagIndex index) const -> DiagKind {
    const auto real = static_cast<std::size_t>(index.get());
    return m_messages.at(real).kind;
}

auto DiagEngine::getDiagnostic(const DiagIndex index) const -> const llvm::SMDiagnostic& {
    const auto real = static_cast<std::size_t>(index.get());
    return m_messages.at(real).diagnostic;
}

auto DiagEngine::getLocation(DiagIndex index) const -> const std::source_location& {
    const auto real = static_cast<std::size_t>(index.get());
    return m_messages.at(real).location;
}

auto DiagEngine::log(
    const DiagMessage& message,
    llvm::SMLoc loc,
    llvm::ArrayRef<llvm::SMRange> ranges,
    std::source_location location
) -> DiagIndex {
    const auto index = m_messages.size();
    auto diagnostic = [&] -> llvm::SMDiagnostic {
        if (loc.isValid()) {
            return m_context.getSourceMgr().GetMessage(loc, message.first.getSeverity(), message.second, ranges);
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
        llvm::outs() << std::format(
            "From {}:{}:{} in \"{}\"\n",
            loc.file_name(),
            loc.line(),
            loc.column(),
            loc.function_name()
        );
    }
}
