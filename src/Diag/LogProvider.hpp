//
// Created by Albert Varaksin on 16/02/2026.
//
#pragma once
#include "pch.hpp"
#include "DiagEngine.hpp"
#include "Driver/Context.hpp"
namespace lbc {

/**
 * Convenience mixin that provides a `diag()` helper for logging
 * diagnostic messages. Uses C++23 deducing this with the
 * `ContextAware` concept — any derived class that exposes
 * `getContext()` returning `Context&` automatically gains
 * access to the diagnostic engine.
 *
 * @code
 * return diag(diagnostics::unexpected(token), loc);
 * @endcode
 */
class LogProvider {
protected:
    /**
     * Log a diagnostic and return the result as a DiagError,
     * suitable for direct use in a return statement.
     *
     * @param message diagnostic message to log
     * @param loc source location of the diagnostic
     * @param ranges optional source ranges to highlight
     * @param location C++ call site, captured automatically
     * @return DiagError wrapping the logged DiagIndex
     */
    template<ContextAware T>
    [[nodiscard]] auto diag(
        this T& self,
        const DiagMessage& message,
        const llvm::ArrayRef<llvm::SMRange>& ranges = {},
        const llvm::SMLoc loc = {},
        const std::source_location& location = std::source_location::current()
    ) -> DiagError {
        return DiagError(self.getContext().getDiag().log(message, ranges, loc, location));
    }

    /**
     * Create an error indicating unimplemented functionality.
     */
    template<ContextAware T>
    [[nodiscard]] auto notImplemented(this T& self, const std::source_location& location = std::source_location::current()) -> DiagError {
        return DiagError(self.getContext().getDiag().log(diagnostics::notImplemented(), {}, {}, location));
    }
};
} // namespace lbc
