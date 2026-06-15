//
// Created by Albert Varaksin on 15/06/2026.
//
#include "Driver.hpp"
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include "Task.hpp"
#include "Toolchain.hpp"
#include "tasks/CodeGenTask.hpp"
#include "tasks/CompileTask.hpp"
#include "tasks/EmitTask.hpp"
#include "tasks/OptimizeTask.hpp"
using namespace lbc;

namespace {
/** Make @p path absolute against @p base and collapse any `.`/`..` segments. */
auto absolutize(const llvm::StringRef path, const llvm::Twine& base) -> std::string {
    llvm::SmallString<256> result { path };
    llvm::sys::path::make_absolute(base, result);
    llvm::sys::path::remove_dots(result, /*remove_dot_dot=*/true);
    return std::string { result.data(), result.size() };
}
} // namespace

Driver::Driver(CompileOptions options)
: m_context(std::move(options)) {
    m_context.getDiag().setVerbose(m_context.getOptions().isVerbose());
}

auto Driver::execute() -> bool {
    return run().has_value();
}

auto Driver::run() -> DiagResult<void> {
    const auto& options = m_context.getOptions();

    if (options.isDumpConfig()) {
        llvm::errs() << options.toCommandLine() << '\n';
    }

    resolvePaths();
    TRY(validate())

    // Everything but a linked executable (which needs the link stage) is wired:
    // LLVM IR in-process, objects and assembly via llc.
    if (options.getOutputType() == CompileOptions::OutputType::Executable) {
        return DiagError { m_context.getDiag().log(diagnostics::notImplemented()) };
    }

    const auto pipeline = buildPipeline();
    for (const auto& source : m_inputs) {
        Unit unit { source, m_output };
        for (const auto& task : pipeline) {
            TRY(task->run(m_context, unit))
        }
    }
    return {};
}

auto Driver::buildPipeline() const -> std::vector<std::unique_ptr<Task>> {
    const auto& options = m_context.getOptions();
    const Toolchain toolchain { options.getToolchainPath().str() };

    std::vector<std::unique_ptr<Task>> pipeline;
    pipeline.push_back(std::make_unique<CompileTask>());

    // Optimise by shelling out to `opt` when any optimisation is requested.
    if (options.getOptimizationLevel() != CompileOptions::OptimizationLevel::O0) {
        pipeline.push_back(std::make_unique<OptimizeTask>(toolchain.getOptimizer()));
    }

    // Emit the requested artifact. Objects go through `llc`; the link stage
    // (consuming each unit's object) slots in here as the backend grows.
    switch (options.getOutputType()) {
    case CompileOptions::OutputType::Object:
    case CompileOptions::OutputType::Assembly:
        pipeline.push_back(std::make_unique<CodeGenTask>(toolchain.getCodeGen()));
        break;
    case CompileOptions::OutputType::LlvmIr:
        pipeline.push_back(std::make_unique<EmitTask>());
        break;
    default:
        break; // executable rejected before this point
    }
    return pipeline;
}

void Driver::resolvePaths() {
    const auto& options = m_context.getOptions();

    // Establish an absolute base directory for relative paths.
    llvm::SmallString<256> base;
    if (options.getWorkingDirectory().empty()) {
        std::ignore = llvm::sys::fs::current_path(base);
    } else {
        base = options.getWorkingDirectory();
        std::ignore = llvm::sys::fs::make_absolute(base);
    }
    llvm::sys::path::remove_dots(base, /*remove_dot_dot=*/true);
    const std::string baseDir { base.data(), base.size() };

    const auto sources = options.getFiles(CompileOptions::FileType::Source);
    m_inputs.reserve(sources.size());
    for (const auto& file : sources) {
        m_inputs.push_back(absolutize(file, baseDir));
    }

    if (!options.getOutputPath().empty()) {
        m_output = absolutize(options.getOutputPath(), baseDir);
    }

    // Hand the resolved include hierarchy to the source manager so includes are
    // searched in precedence order when sources reference other files.
    std::vector<std::string> includeDirs;
    includeDirs.reserve(options.getIncludePaths().size());
    for (const auto& dir : options.getIncludePaths()) {
        includeDirs.push_back(absolutize(dir, baseDir));
    }
    m_context.getSourceMgr().setIncludeDirs(includeDirs);
}

auto Driver::validate() -> DiagResult<void> {
    auto& diag = m_context.getDiag();
    const auto& options = m_context.getOptions();

    DiagIndex firstError {};
    const auto report = [&](const DiagMessage& message) {
        const auto index = diag.log(message);
        if (!firstError.isValid()) {
            firstError = index;
        }
    };

    // There must be source files to compile. (Link-only inputs — objects and
    // libraries without sources — await the linker step.)
    if (options.getFiles(CompileOptions::FileType::Source).empty()) {
        report(diagnostics::noInputFiles());
    }

    // Every input file must be reachable.
    for (const auto& file : m_inputs) {
        if (!llvm::sys::fs::exists(file)) {
            report(diagnostics::inputFileNotFound(file));
        }
    }

    // Only an executable links several inputs into one artifact; any other
    // output kind produces one artifact per input and cannot share a path.
    if (options.getOutputType() != CompileOptions::OutputType::Executable
        && !options.getOutputPath().empty()
        && options.getFiles(CompileOptions::FileType::Source).size() > 1) {
        report(diagnostics::ambiguousOutput());
    }

    if (firstError.isValid()) {
        return DiagError { firstError };
    }
    return {};
}
