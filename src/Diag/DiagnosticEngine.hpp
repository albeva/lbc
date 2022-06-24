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
    static std::string format(Diag diag, Args&&... args) noexcept {
        return llvm::formatv(getText(diag), std::forward<Args>(args)...).str();
    }

    void print(Diag diag, llvm::SMLoc loc, const std::string& str, llvm::ArrayRef<llvm::SMRange> ranges = {}) noexcept;

    template<std::invocable Func>
    inline auto ignoringErrors(Func&& func) -> std::invoke_result_t<Func> {
        RESTORE_ON_EXIT(m_ignoreErrors);
        m_ignoreErrors = true;
        return func();
    }

    template<typename... Args>
    [[nodiscard]] ResultError makeError(Diag diag, const llvm::SMRange& range, Args&&... args) noexcept {
        print(
            diag,
            range.Start,
            format(diag, std::forward<Args>(args)...),
            range);
        return ResultError{};
    }

private:
    static const char* getText(Diag diag) noexcept;
    static llvm::SourceMgr::DiagKind getKind(Diag diag) noexcept;

    Context& m_context;
    llvm::SourceMgr& m_sourceMgr;
    int m_errorCounter = 0;
    bool m_ignoreErrors = false;
};

} // namespace lbc