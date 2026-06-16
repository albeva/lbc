//
// Created by Albert Varaksin on 15/06/2026.
//
#pragma once
#include "pch.hpp"
#include <vector>
#include "Context.hpp"

namespace lbc {

/**
 * Orchestrates a single compilation.
 *
 * The Driver owns the Context (and through it the frozen CompileOptions). It
 * validates the configuration, resolves the input and include paths over the
 * configured search hierarchy, then drives every input source through the
 * compilation stages.
 *
 * Each input flows through the stages compile → [write bitcode → optimise →
 * emit native]; producing an executable then links the per-source objects
 * together. The in-process LLVM IR path, the native object/assembly paths, and
 * executable linking are all wired.
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

    /** Drive one resolved source through the stages; returns the artifact it produced. */
    [[nodiscard]] auto compileSource(const std::string& source) -> DiagResult<std::string>;

    Context m_context;                 ///< owns the options and all per-compilation state
    std::vector<std::string> m_inputs; ///< resolved absolute input paths
};

} // namespace lbc
