//
// Created by Albert Varaksin on 15/06/2026.
//
#include "CompileOptions.hpp"
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
using namespace lbc;

namespace {
/** Resolve @p path to an absolute, normalised directory; empty resolves to the CWD. */
auto resolveDirectory(const llvm::StringRef path) -> std::string {
    llvm::SmallString<256> dir;
    if (path.empty()) {
        std::ignore = llvm::sys::fs::current_path(dir);
    } else {
        dir = path;
        std::ignore = llvm::sys::fs::make_absolute(dir);
    }
    llvm::sys::path::remove_dots(dir, /*remove_dot_dot=*/true);
    return std::string { dir.data(), dir.size() };
}

/** File extension (with leading dot) for an output kind; empty for an executable. */
auto outputExtension(const CompileOptions::OutputType type) -> llvm::StringRef {
    using Out = CompileOptions::OutputType;
    switch (type) {
    case Out::Object:
        return ".o";
    case Out::Assembly:
        return ".s";
    case Out::LlvmIr:
        return ".ll";
    case Out::Executable:
        return ""; // platform-specific naming is handled once linking lands
    }
    return "";
}

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

auto platformFlag(const CompileOptions::Platform platform) -> llvm::StringRef {
    using Platform = CompileOptions::Platform;
    switch (platform) {
    case Platform::Default:
        return {};
    case Platform::Linux:
        return "--platform=linux";
    case Platform::Windows:
        return "--platform=windows";
    case Platform::MacOS:
        return "--platform=macos";
    }
    return {};
}
} // namespace

void CompileOptions::addFile(const llvm::StringRef path) {
    addFile(detectFileType(path), path);
}

void CompileOptions::finalize() {
    // Resolve the working directory to an absolute, normalised path (CWD if unset).
    m_workingDirectory = resolveDirectory(m_workingDirectory);

    // Derive the output path from the first input when none was given explicitly.
    if (m_outputPath.empty()) {
        m_outputPath = deriveDefaultOutput();
    }
}

auto CompileOptions::deriveDefaultOutput() const -> std::string {
    const auto sources = getFiles(FileType::Source);
    const auto objects = getFiles(FileType::Object);

    llvm::StringRef first;
    if (!sources.empty()) {
        first = sources.front();
    } else if (!objects.empty()) {
        first = objects.front();
    } else {
        return {}; // no input to derive from; the driver reports the missing input
    }

    const std::string name = (llvm::Twine(llvm::sys::path::stem(first)) + outputExtension(m_outputType)).str();
    if (m_workingDirectory.empty()) {
        return name;
    }
    llvm::SmallString<256> path { m_workingDirectory };
    llvm::sys::path::append(path, name);
    return std::string { path.data(), path.size() };
}

auto CompileOptions::getOptimizationFlag() const -> llvm::StringRef {
    switch (m_optimizationLevel) {
    case OptimizationLevel::O0:
        return "-O0";
    case OptimizationLevel::O1:
        return "-O1";
    case OptimizationLevel::O2:
        return "-O2";
    case OptimizationLevel::O3:
        return "-O3";
    case OptimizationLevel::Os:
        return "-Os";
    case OptimizationLevel::Oz:
        return "-Oz";
    }
    return "-O0";
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
        append(getOptimizationFlag());
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
    if (m_platform != Platform::Default) {
        append(platformFlag(m_platform));
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
    if (m_verbose) {
        append("--verbose");
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
