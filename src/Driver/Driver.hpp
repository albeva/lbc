//
// Created by Albert Varaksin on 15/06/2026.
//
#pragma once
#include "pch.hpp"
#include "Context.hpp"

namespace llvm {
class Module;
}

namespace lbc {

/**
 * Orchestrates a single compilation.
 *
 * The Driver owns the Context (and through it the frozen CompileOptions). It
 * validates the configuration, resolves the input and include paths over the
 * configured search hierarchy, then drives every input source through the
 * frontend → IR → LLVM pipeline to the requested output.
 *
 * CompileOptions is a pure data model; all behaviour — path resolution,
 * validation, pipeline orchestration, emission — lives here.
 */
class Driver final {
public:
    NO_COPY_AND_MOVE(Driver)

    /** Take ownership of the options for this compilation. */
    explicit Driver(CompileOptions options);

    /** Run the whole compilation. Returns true on success, false on any error. */
    [[nodiscard]] auto execute() -> bool;

private:
    /** The pipeline proper, expressed with DiagResult propagation. */
    [[nodiscard]] auto run() -> DiagResult<void>;

    /** Resolve the working directory, inputs, output, and include search dirs. */
    void resolvePaths();

    /** Reject an inconsistent configuration or unreachable inputs. */
    [[nodiscard]] auto validate() -> DiagResult<void>;

    /** Compile a single resolved input file. */
    [[nodiscard]] auto compile(const std::string& path) -> DiagResult<void>;

    /** Verify and write the lowered LLVM module to the output. */
    [[nodiscard]] auto emit(llvm::Module& module) -> DiagResult<void>;

    Context m_context;                 ///< owns the options and all per-compilation state
    std::vector<std::string> m_inputs; ///< resolved absolute input paths
    std::string m_output;              ///< resolved output path, empty for stdout
};

} // namespace lbc
