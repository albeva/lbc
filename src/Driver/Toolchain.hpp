//
// Created by Albert Varaksin on 15/06/2026.
//
#pragma once
#include "pch.hpp"
#include "Diag/DiagEngine.hpp"

namespace lbc {
class Context;

/**
 * Resolves the external toolchain binaries the driver shells out to (optimiser,
 * code generator, linker). Encapsulates tool names and platform quirks such as
 * the `.exe` suffix on Windows, and reports a diagnostic when a tool cannot be
 * found.
 *
 * The LLVM tools resolve to `<toolchain-dir>/<tool>[.exe]` when a toolchain
 * directory is configured, otherwise they are looked up on PATH. The linker is
 * always the host `cc` from PATH.
 */
class Toolchain final {
public:
    /** @param context supplies the toolchain directory (via options) and diagnostics. */
    explicit Toolchain(Context& context)
    : m_context(context) {}

    /** Path to the LLVM optimiser (`opt`), or a diagnostic if it cannot be found. */
    [[nodiscard]] auto getOptimizer() const -> DiagResult<std::string> { return resolve("opt"); }

    /** Path to the LLVM code generator (`llc`), or a diagnostic if it cannot be found. */
    [[nodiscard]] auto getCodeGen() const -> DiagResult<std::string> { return resolve("llc"); }

    /** Path to the C compiler driver used to link (host `cc`), or a diagnostic if not found. */
    [[nodiscard]] auto getLinker() const -> DiagResult<std::string>;

private:
    /** Resolve a tool name to an existing executable path, or report `toolNotFound`. */
    [[nodiscard]] auto resolve(llvm::StringRef tool) const -> DiagResult<std::string>;

    Context& m_context; ///< supplies the toolchain directory and the diagnostic engine
};

} // namespace lbc
