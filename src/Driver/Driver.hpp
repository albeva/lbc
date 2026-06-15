//
// Created by Albert Varaksin on 15/06/2026.
//
#pragma once
#include "pch.hpp"
#include <memory>
#include <vector>
#include "Context.hpp"

namespace lbc {
class Task;

/**
 * Orchestrates a single compilation.
 *
 * The Driver owns the Context (and through it the frozen CompileOptions). It
 * validates the configuration, resolves the input and include paths over the
 * configured search hierarchy, builds a pipeline of tasks from the options, and
 * drives every input source through it.
 *
 * Each input flows through the stages compile → [optimise] → emit; producing an
 * executable additionally links the per-source artifacts together. Today only
 * the in-process compile → emit (LLVM IR) stages are wired.
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

    /** Assemble the ordered stages each input is driven through. */
    [[nodiscard]] auto buildPipeline() const -> std::vector<std::unique_ptr<Task>>;

    Context m_context;                 ///< owns the options and all per-compilation state
    std::vector<std::string> m_inputs; ///< resolved absolute input paths
    std::string m_output;              ///< resolved output path, empty for stdout
};

} // namespace lbc
