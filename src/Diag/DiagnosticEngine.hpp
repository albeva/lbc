//
// Created by Albert on 03/06/2021.
//
#pragma once

namespace lbc {
class Context;

enum class Diag {
#define DIAG(LEVEL, ID, ...) ID,
#include "Diagnostics.def.hpp"
};

class DiagnosticEngine final {
public:
    NO_COPY_AND_MOVE(DiagnosticEngine)

    explicit DiagnosticEngine(Context& context) noexcept;
    ~DiagnosticEngine() noexcept = default;

    [[nodiscard]] bool hasErrors() const noexcept { return m_errorCounter > 0; }

    template<typename... Args>
    static string format(Diag diag, Args&&... args) noexcept {
        return llvm::formatv(getText(diag), std::forward<Args>(args)...).str();
    }

    void print(Diag diag, llvm::SMLoc loc, const string& str, llvm::ArrayRef<llvm::SMRange> ranges = {}) noexcept;

private:
    static const char* getText(Diag diag) noexcept;
    static llvm::SourceMgr::DiagKind getKind(Diag diag) noexcept;

    Context& m_context;
    llvm::SourceMgr& m_sourceMgr;
    int m_errorCounter = 0;
};

} // namespace lbc