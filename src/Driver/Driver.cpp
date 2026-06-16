//
// Created by Albert Varaksin on 15/06/2026.
//
#include "Driver.hpp"
#include <llvm/ADT/SmallString.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include "tasks/CompileTask.hpp"
#include "tasks/EmitBinaryTask.hpp"
#include "tasks/EmitLlvmTask.hpp"
#include "tasks/EmitNativeTask.hpp"
#include "tasks/OptimizeTask.hpp"
#include "tasks/WriteBitcodeTask.hpp"
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

    // Compile each source to its artifact (an object file when an executable is
    // being built), collecting the produced paths.
    std::vector<std::string> artifacts;
    artifacts.reserve(m_inputs.size());
    for (const auto& source : m_inputs) {
        TRY_DECL(artifact, compileSource(source))
        artifacts.push_back(std::move(artifact));
    }

    // Link the generated objects into the final executable.
    if (options.getOutputType() == CompileOptions::OutputType::Executable) {
        EmitBinaryTask link { m_context };
        TRY(link.run(std::move(artifacts)))
    }
    return {};
}

auto Driver::compileSource(const std::string& source) -> DiagResult<std::string> {
    // LLVM IR is emitted straight from the in-memory module; native output
    // serialises the module to bitcode once, then flows that file through the
    // shell-out stages (OptimizeTask is a no-op at -O0). Either way the final
    // stage returns the path it wrote.
    if (m_context.getOptions().getOutputType() == CompileOptions::OutputType::LlvmIr) {
        return pipeline<CompileTask, EmitLlvmTask>(m_context, source);
    }
    return pipeline<CompileTask, WriteBitcodeTask, OptimizeTask, EmitNativeTask>(m_context, source);
}

void Driver::resolvePaths() {
    const auto& options = m_context.getOptions();

    // The working directory is already resolved to an absolute path by the options.
    const llvm::StringRef baseDir = options.getWorkingDirectory();

    const auto sources = options.getFiles(CompileOptions::FileType::Source);
    m_inputs.reserve(sources.size());
    for (const auto& file : sources) {
        m_inputs.push_back(absolutize(file, baseDir));
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
    // output kind has a single output path that multiple sources cannot share.
    if (options.getOutputType() != CompileOptions::OutputType::Executable
        && options.getFiles(CompileOptions::FileType::Source).size() > 1) {
        report(diagnostics::ambiguousOutput());
    }

    if (firstError.isValid()) {
        return DiagError { firstError };
    }
    return {};
}
