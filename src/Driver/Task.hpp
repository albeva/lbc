//
// Created by Albert Varaksin on 15/06/2026.
//
#pragma once
#include "pch.hpp"
#include "Diag/DiagEngine.hpp"
#include "Utilities/Try.hpp"

namespace lbc {
class Context;

/**
 * How a task names its output artefact. An empty @ref baseName requests a
 * temporary (deleted once consumed); otherwise the output is written to the
 * build path as `<baseName>.<ext>` — the task supplies the extension.
 */
struct TaskOption final {
    std::string baseName; ///< output base name; empty requests a temporary

    /** Whether the output should be a temporary (no base name was given). */
    [[nodiscard]] auto isTemporary() const -> bool { return baseName.empty(); }
};

/**
 * A single compilation stage: transforms an @p Input artifact into an @p Output
 * artifact, or fails with a diagnostic. Stages read what they need from the
 * Context handed to @ref run, so @ref pipeline can thread one Context — and each
 * stage's output — through a sequence of stage instances.
 */
template<typename Input, typename Output>
class Task {
public:
    using InputType = Input;
    using OutputType = Output;

    NO_COPY_AND_MOVE(Task)
    Task() = default;
    virtual ~Task() = default;

    /** Run the stage on @p input within @p context, producing its output. */
    [[nodiscard]] virtual auto run(Context& context, Input input) -> DiagResult<Output> = 0;
};

namespace detail {
    /** The @ref Task::OutputType of the last task in the pack — a pipeline's result. */
    template<typename... Tasks>
    struct LastOutput;

    template<typename Task>
    struct LastOutput<Task> {
        using type = typename std::remove_cvref_t<Task>::OutputType;
    };

    template<typename Task, typename... Rest>
    struct LastOutput<Task, Rest...> : LastOutput<Rest...> {};
} // namespace detail

/**
 * Run @p input through @p tasks in order, feeding each stage's output into the
 * next. Short-circuits on the first failure, propagating its diagnostic;
 * otherwise the result is the last stage's output. Stages are passed as
 * instances, so each can carry its own constructor arguments.
 *
 * @code
 * TRY_DECL(object, pipeline(context, source, CompileTask{}, WriteBitcodeTask{}, EmitNativeTask{}))
 * @endcode
 */
template<typename Input, typename First, typename... Rest>
[[nodiscard]] auto pipeline(Context& context, Input input, First&& first, Rest&&... rest)
    -> DiagResult<typename detail::LastOutput<First, Rest...>::type> {
    if constexpr (sizeof...(Rest) == 0) {
        return first.run(context, std::move(input));
    } else {
        TRY_DECL(next, first.run(context, std::move(input)))
        return pipeline(context, std::move(next), std::forward<Rest>(rest)...);
    }
}

} // namespace lbc
