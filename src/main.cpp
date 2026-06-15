#include "pch.hpp"
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/InitLLVM.h>

#include "Driver/CompileOptions.hpp"
#include "Driver/Driver.hpp"

namespace {
namespace cl = llvm::cl;
using lbc::CompileOptions;

cl::OptionCategory lbcCategory { "lbc options" };

cl::list<std::string> inputFiles(
    cl::Positional,
    cl::desc("<input files>"),
    cl::cat(lbcCategory)
);

cl::list<std::string> includeDirs(
    "I",
    cl::desc("Add <dir> to the include search path"),
    cl::value_desc("dir"),
    cl::cat(lbcCategory)
);

cl::opt<std::string> outputPath(
    "o",
    cl::desc("Write output to <file> (default: stdout)"),
    cl::value_desc("file"),
    cl::cat(lbcCategory)
);

cl::opt<std::string> workingDir(
    "working-dir",
    cl::desc("Resolve relative paths against <dir> (default: current directory)"),
    cl::value_desc("dir"),
    cl::cat(lbcCategory)
);

cl::opt<std::string> toolchainDir(
    "toolchain",
    cl::desc("Directory holding the LLVM toolchain binaries"),
    cl::value_desc("dir"),
    cl::cat(lbcCategory)
);

cl::opt<bool> debugInfo("g", cl::desc("Emit debug information"), cl::cat(lbcCategory));

cl::opt<bool> dumpAst("dump-ast", cl::desc("Dump the parsed AST to stderr"), cl::cat(lbcCategory));
cl::opt<bool> dumpIr("dump-ir", cl::desc("Dump the lbc IR to stderr"), cl::cat(lbcCategory));
cl::opt<bool> dumpConfig("dump-config", cl::desc("Dump the options as a command line to stderr"), cl::cat(lbcCategory));

cl::opt<CompileOptions::OptimizationLevel> optLevel(
    cl::desc("Optimisation level:"),
    cl::values(
        clEnumValN(CompileOptions::OptimizationLevel::O0, "O0", "No optimisation (default)"),
        clEnumValN(CompileOptions::OptimizationLevel::O1, "O1", "Basic optimisations"),
        clEnumValN(CompileOptions::OptimizationLevel::O2, "O2", "Full optimisations"),
        clEnumValN(CompileOptions::OptimizationLevel::O3, "O3", "Aggressive optimisations"),
        clEnumValN(CompileOptions::OptimizationLevel::Os, "Os", "Optimise for size"),
        clEnumValN(CompileOptions::OptimizationLevel::Oz, "Oz", "Optimise aggressively for size")
    ),
    cl::init(CompileOptions::OptimizationLevel::O0),
    cl::cat(lbcCategory)
);

cl::opt<CompileOptions::OutputType> emit(
    "emit",
    cl::desc("Output kind:"),
    cl::values(
        clEnumValN(CompileOptions::OutputType::Executable, "exe", "Linked executable (default)"),
        clEnumValN(CompileOptions::OutputType::Object, "obj", "Object file"),
        clEnumValN(CompileOptions::OutputType::Assembly, "asm", "Native assembly"),
        clEnumValN(CompileOptions::OutputType::LlvmIr, "llvm", "Textual LLVM IR")
    ),
    cl::init(CompileOptions::OutputType::Executable),
    cl::cat(lbcCategory)
);

cl::opt<CompileOptions::Arch> targetArch(
    "arch",
    cl::desc("Target architecture (default: host):"),
    cl::values(
        clEnumValN(CompileOptions::Arch::X86, "x86", "Intel / AMD x86"),
        clEnumValN(CompileOptions::Arch::Arm, "arm", "ARM"),
        clEnumValN(CompileOptions::Arch::RiscV, "riscv", "RISC-V"),
        clEnumValN(CompileOptions::Arch::PowerPc, "powerpc", "PowerPC"),
        clEnumValN(CompileOptions::Arch::Mips, "mips", "MIPS"),
        clEnumValN(CompileOptions::Arch::Wasm, "wasm", "WebAssembly"),
        clEnumValN(CompileOptions::Arch::C, "c", "Portable C source")
    ),
    cl::init(CompileOptions::Arch::Default),
    cl::cat(lbcCategory)
);

cl::opt<CompileOptions::Bitness> targetBits(
    "bits",
    cl::desc("Target pointer width (default: host):"),
    cl::values(
        clEnumValN(CompileOptions::Bitness::Bits32, "32", "32-bit"),
        clEnumValN(CompileOptions::Bitness::Bits64, "64", "64-bit")
    ),
    cl::init(CompileOptions::Bitness::Default),
    cl::cat(lbcCategory)
);

/** Assemble a CompileOptions from the parsed command-line state. */
[[nodiscard]] auto buildOptions(const std::string& compilerPath) -> CompileOptions {
    CompileOptions options;
    options.setCompilerPath(compilerPath);
    for (const auto& file : inputFiles) {
        options.addFile(file); // bucketed by extension
    }
    for (const auto& dir : includeDirs) {
        options.addIncludePath(dir);
    }
    options.setOutputPath(outputPath);
    options.setWorkingDirectory(workingDir);
    options.setToolchainPath(toolchainDir);
    options.setOutputType(emit);
    options.setOptimizationLevel(optLevel);
    options.setArch(targetArch);
    options.setBitness(targetBits);
    options.setDebugInfo(debugInfo);
    options.setDumpAst(dumpAst);
    options.setDumpIr(dumpIr);
    options.setDumpConfig(dumpConfig);
    return options;
}
} // namespace

auto main(int argc, const char* argv[]) -> int {
    llvm::InitLLVM const init { argc, argv };
    cl::HideUnrelatedOptions(lbcCategory);
    if (!cl::ParseCommandLineOptions(argc, argv, "lbc - the lbc BASIC compiler\n", &llvm::errs())) {
        return 1;
    }

    auto addr = reinterpret_cast<void*>(reinterpret_cast<std::intptr_t>(&buildOptions));
    const auto executable = llvm::sys::fs::getMainExecutable(argv[0], addr);
    lbc::Driver driver { buildOptions(executable) };
    return driver.execute() ? 0 : 1;
}
