//
// Created by Albert Varaksin on 15/06/2026.
//
#pragma once
#include "pch.hpp"
#include <llvm/IR/Module.h>
#include "Diag/DiagEngine.hpp"

namespace lbc {
class Context;

/**
 * The evolving build state for a single input as it travels through the task
 * pipeline. Each Task reads the unit's current artifacts and advances them —
 * CompileTask fills @ref module, CodeGenTask records @ref objectPath, and so on.
 * The module lives in the Context's LLVM context, which outlives every unit.
 */
struct Unit final {
    NO_COPY_AND_MOVE(Unit)

    Unit(std::string source, std::string output)
    : sourcePath(std::move(source))
    , outputPath(std::move(output)) {}

    ~Unit() = default;

    std::string sourcePath;               ///< absolute path to the input source
    std::string outputPath;               ///< destination path, empty means stdout
    std::string objectPath;               ///< emitted object file (set by CodeGenTask)
    std::unique_ptr<llvm::Module> module; ///< lowered LLVM module (set by CompileTask)
};

/**
 * One stage of the compilation pipeline. Stages are stateless; all per-input
 * state lives in the Unit, so a single Task instance can drive many units.
 */
class Task {
public:
    NO_COPY_AND_MOVE(Task)
    Task() = default;
    virtual ~Task() = default;

    /** Advance @p unit through this stage. */
    [[nodiscard]] virtual auto run(Context& context, Unit& unit) -> DiagResult<void> = 0;
};

} // namespace lbc
