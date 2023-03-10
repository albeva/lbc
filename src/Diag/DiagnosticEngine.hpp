//
// Created by Albert on 03/06/2021.
//
#pragma once
#include "pch.hpp"

namespace lbc {
class Context;

enum class Diag {
#define DIAG(LEVEL, ID, ...) ID,
#include "Diagnostics.def.hpp"
};

class DiagnosticEngine final {
public:
    NO_COPY_AND_MOVE(DiagnosticEngine)

    explicit DiagnosticEngine(Context& context) ;
    ~DiagnosticEngine() = default;

    [[nodiscard]] bool hasErrors() const { return m_errorCounter > 0; }

    template<typename... Args>
    static std::string format(Diag diag, Args&&... args) {
        return llvm::formatv(getText(diag), std::forward<Args>(args)...).str();
    }

    void print(Diag diag, llvm::SMLoc loc, const std::string& str, llvm::ArrayRef<llvm::SMRange> ranges = {}) ;

    template<std::invocable Func>
    inline auto ignoringErrors(Func&& func) -> std::invoke_result_t<Func> {
        RESTORE_ON_EXIT(m_ignoreErrors);
        m_ignoreErrors = true;
        return func();
    }

    template<typename... Args>
    [[nodiscard]] ResultError makeError(Diag diag, const llvm::SMRange& range, Args&&... args) {
        print(
            diag,
            range.Start,
            format(diag, std::forward<Args>(args)...),
            range);
        return ResultError{};
    }

private:
    static const char* getText(Diag diag) ;
    static llvm::SourceMgr::DiagKind getKind(Diag diag) ;

    Context& m_context;
    llvm::SourceMgr& m_sourceMgr;
    int m_errorCounter = 0;
    bool m_ignoreErrors = false;
};

} // namespace lbc