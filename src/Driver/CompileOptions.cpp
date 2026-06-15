//
// Created by Albert Varaksin on 15/06/2026.
//
#include "CompileOptions.hpp"
#include <llvm/ADT/StringSwitch.h>
#include <llvm/Support/Path.h>
using namespace lbc;

namespace {
/** Classify a file by its path extension; anything unknown is treated as source. */
auto detectFileType(const llvm::StringRef path) -> CompileOptions::FileType {
    using FileType = CompileOptions::FileType;
    return llvm::StringSwitch<FileType>(llvm::sys::path::extension(path).lower())
        .Cases({ ".s", ".asm" }, FileType::Assembly)
        .Cases({ ".o", ".obj" }, FileType::Object)
        .Cases({ ".a", ".lib", ".so", ".dylib", ".dll" }, FileType::Library)
        .Default(FileType::Source);
}

/** Wrap a token in double quotes when it contains whitespace. */
auto quote(const llvm::StringRef token) -> std::string {
    if (token.contains(' ')) {
        return "\"" + token.str() + "\"";
    }
    return token.str();
}

auto optFlag(const CompileOptions::OptimizationLevel level) -> llvm::StringRef {
    using Opt = CompileOptions::OptimizationLevel;
    switch (level) {
    case Opt::O0:
        return "-O0";
    case Opt::O1:
        return "-O1";
    case Opt::O2:
        return "-O2";
    case Opt::O3:
        return "-O3";
    case Opt::Os:
        return "-Os";
    case Opt::Oz:
        return "-Oz";
    }
    return {};
}

auto emitFlag(const CompileOptions::OutputType type) -> llvm::StringRef {
    using Out = CompileOptions::OutputType;
    switch (type) {
    case Out::Executable:
        return "--emit=exe";
    case Out::Object:
        return "--emit=obj";
    case Out::Assembly:
        return "--emit=asm";
    case Out::LlvmIr:
        return "--emit=llvm";
    }
    return {};
}

auto archFlag(const CompileOptions::Arch arch) -> llvm::StringRef {
    using Arch = CompileOptions::Arch;
    switch (arch) {
    case Arch::Default:
        return {};
    case Arch::X86:
        return "--arch=x86";
    case Arch::Arm:
        return "--arch=arm";
    case Arch::RiscV:
        return "--arch=riscv";
    case Arch::PowerPc:
        return "--arch=powerpc";
    case Arch::Mips:
        return "--arch=mips";
    case Arch::Wasm:
        return "--arch=wasm";
    case Arch::C:
        return "--arch=c";
    }
    return {};
}

auto bitnessFlag(const CompileOptions::Bitness bitness) -> llvm::StringRef {
    using Bitness = CompileOptions::Bitness;
    switch (bitness) {
    case Bitness::Default:
        return {};
    case Bitness::Bits32:
        return "--bits=32";
    case Bitness::Bits64:
        return "--bits=64";
    }
    return {};
}
} // namespace

void CompileOptions::addFile(const llvm::StringRef path) {
    addFile(detectFileType(path), path);
}

auto CompileOptions::toCommandLine() const -> std::string {
    std::string result;
    const auto append = [&](const llvm::StringRef token) {
        if (!result.empty()) {
            result += ' ';
        }
        result.append(token.begin(), token.end());
    };
    const auto appendPath = [&](const llvm::StringRef flag, const llvm::StringRef path) {
        append(flag);
        append(quote(path));
    };

    append(quote(m_compilerPath.empty() ? llvm::StringRef { "lbc" } : llvm::StringRef { m_compilerPath }));

    // Only non-default settings are emitted, so the line reads like a real invocation.
    if (m_optimizationLevel != OptimizationLevel::O0) {
        append(optFlag(m_optimizationLevel));
    }
    if (m_debugInfo) {
        append("-g");
    }
    if (m_outputType != OutputType::Executable) {
        append(emitFlag(m_outputType));
    }
    if (m_arch != Arch::Default) {
        append(archFlag(m_arch));
    }
    if (m_bitness != Bitness::Default) {
        append(bitnessFlag(m_bitness));
    }
    if (!m_workingDirectory.empty()) {
        appendPath("--working-dir", m_workingDirectory);
    }
    if (!m_toolchainPath.empty()) {
        appendPath("--toolchain", m_toolchainPath);
    }
    for (const auto& include : m_includePaths) {
        appendPath("-I", include);
    }
    if (m_dumpAst) {
        append("--dump-ast");
    }
    if (m_dumpIr) {
        append("--dump-ir");
    }
    if (m_dumpConfig) {
        append("--dump-config");
    }
    if (!m_outputPath.empty()) {
        appendPath("-o", m_outputPath);
    }

    // Input files as positionals (the parser re-buckets them by extension).
    for (const auto& bucket : m_files) {
        for (const auto& file : bucket) {
            append(quote(file));
        }
    }

    return result;
}
