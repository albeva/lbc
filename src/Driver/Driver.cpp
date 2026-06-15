//
// Created by Albert Varaksin on 15/06/2026.
//
#include "Driver.hpp"
#include <llvm/ADT/SmallString.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include "Ast/AstCodePrinter.hpp"
#include "Gen/Generator.hpp"
#include "IR/gen/IrGenerator.hpp"
#include "IR/printer/Printer.hpp"
#include "Parser/Parser.hpp"
#include "Sema/SemanticAnalyser.hpp"
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
: m_context(std::move(options)) {}

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

    // The backend can currently only emit textual LLVM IR; richer outputs
    // (object, assembly, executable) arrive with native codegen and linking.
    if (options.getOutputType() != CompileOptions::OutputType::LlvmIr) {
        return DiagError { m_context.getDiag().log(diagnostics::notImplemented()) };
    }

    for (const auto& input : m_inputs) {
        TRY(compile(input))
    }
    return {};
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

    bool failed = false;
    DiagIndex firstError {};
    const auto report = [&](const DiagMessage& message) {
        const auto index = diag.log(message);
        if (!failed) {
            firstError = index;
            failed = true;
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

    if (failed) {
        return DiagError { firstError };
    }
    return {};
}

auto Driver::compile(const std::string& path) -> DiagResult<void> {
    const auto& options = m_context.getOptions();

    std::string included;
    const auto id = m_context.getSourceMgr().AddIncludeFile(path, {}, included);
    if (id == 0) {
        return DiagError { m_context.getDiag().log(diagnostics::inputFileNotFound(path)) };
    }

    Parser parser { m_context, id };
    TRY_DECL(module, parser.parse())

    SemanticAnalyser sema { m_context };
    TRY(sema.analyse(*module))

    // Debug dumps go to stderr so they never pollute the artifact on stdout.
    if (options.isDumpAst()) {
        AstCodePrinter { llvm::errs() }.print(*module);
    }

    ir::gen::IrGenerator irGenerator { m_context };
    TRY_DECL(ir, irGenerator.generate(*module))

    if (options.isDumpIr()) {
        ir::printer::Printer { llvm::errs() }.print(*ir);
    }

    gen::Generator generator {};
    auto& llvmModule = generator.generate(*ir);
    TRY(emit(llvmModule))

    return {};
}

auto Driver::emit(llvm::Module& module) -> DiagResult<void> {
    if (llvm::verifyModule(module, &llvm::errs())) {
        return DiagError { m_context.getDiag().log(diagnostics::backendVerificationFailed()) };
    }

    if (m_output.empty()) {
        module.print(llvm::outs(), nullptr);
        return {};
    }

    std::error_code error;
    llvm::raw_fd_ostream out { m_output, error };
    if (error) {
        return DiagError { m_context.getDiag().log(diagnostics::cannotOpenOutput(m_output, error.message())) };
    }
    module.print(out, nullptr);
    return {};
}
