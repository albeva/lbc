//
// Created by Albert Varaksin on 15/06/2026.
//
#pragma once
#include "pch.hpp"

namespace lbc {

/**
 * Resolves the external toolchain binaries the driver shells out to (optimiser,
 * linker, ...). Encapsulates tool names and platform quirks such as the `.exe`
 * suffix on Windows.
 *
 * Given a toolchain directory, a tool resolves to `<dir>/<tool>[.exe]`. When no
 * directory is configured the tool is looked up on PATH.
 */
class Toolchain final {
public:
    /** @param path toolchain directory, or empty to search PATH. */
    explicit Toolchain(std::string path)
    : m_path(std::move(path)) {}

    /** Full path to the LLVM optimiser (`opt`). */
    [[nodiscard]] auto getOptimizer() const -> std::string { return resolve("opt"); }

    /** Full path to the LLVM code generator (`llc`). */
    [[nodiscard]] auto getCodeGen() const -> std::string { return resolve("llc"); }

private:
    /** Resolve a tool name to an executable path. */
    [[nodiscard]] auto resolve(llvm::StringRef tool) const -> std::string;

    std::string m_path; ///< toolchain directory, empty means search PATH
};

} // namespace lbc
