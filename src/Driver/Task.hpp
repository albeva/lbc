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
 * A single compilation stage: transforms an @p Input artifact into an @p Output
 * artifact, or fails with a diagnostic. A stage holds only the Context it runs
 * in (taken at construction) — everything else it needs is read from there — so
 * @ref pipeline can compose stages by type (compile → write bitcode → optimise →
 * codegen → link). @p Output may be `void` for a terminal stage that only emits.
 */
template<typename Input, typename Output>
class Task {
public:
    using InputType = Input;
    using OutputType = Output;

    NO_COPY_AND_MOVE(Task)
    explicit Task(Context& context)
    : m_context(context) {}
    virtual ~Task() = default;

    /** Run the stage on @p input, producing its output (or a diagnostic). */
    [[nodiscard]] virtual auto run(Input input) -> DiagResult<Output> = 0;

protected:
    Context& m_context; ///< the compilation context this stage runs in
};

namespace detail {
    /** The @ref Task::OutputType of the last task in the pack — a pipeline's result. */
    template<typename... Tasks>
    struct LastOutput;

    template<typename Task>
    struct LastOutput<Task> {
        using type = typename Task::OutputType;
    };

    template<typename Task, typename... Rest>
    struct LastOutput<Task, Rest...> : LastOutput<Rest...> {};
} // namespace detail

/**
 * Run @p input through @p Tasks in order, default-constructing each stage and
 * feeding its output into the next. The chain short-circuits on the first
 * failure, propagating that diagnostic; otherwise the result is the last
 * stage's output (`void` for a terminal stage).
 *
 * @code
 * TRY_DECL(object, pipeline<CompileTask, WriteBitcodeTask, EmitNativeTask>(context, source))
 * @endcode
 */
template<typename First, typename... Rest, typename Input>
[[nodiscard]] auto pipeline(Context& context, Input input)
    -> DiagResult<typename detail::LastOutput<First, Rest...>::type> {
    First stage { context };
    if constexpr (sizeof...(Rest) == 0) {
        return stage.run(std::move(input));
    } else {
        TRY_DECL(next, stage.run(std::move(input)))
        return pipeline<Rest...>(context, std::move(next));
    }
}

} // namespace lbc
