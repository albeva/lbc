//
// Created by Albert Varaksin on 15/06/2026.
//
#pragma once
#include "pch.hpp"
#include <vector>

namespace lbc {

/**
 * The authoritative description of a single compilation: what to compile, how
 * to compile it, for which target, and what to produce.
 *
 * A pure data model — it only holds configuration and exposes accessors. It
 * performs no path resolution or validation of its own; the Driver does that,
 * walking the include-path hierarchy held here. Built once (manually today,
 * from the command line later), then moved into the Context, where every stage
 * can read it but none may change it.
 */
class CompileOptions final {
public:
    /** How an input file is treated by the pipeline. */
    enum class FileType : std::uint8_t {
        Source,   ///< lbc BASIC source to compile
        Assembly, ///< native assembly (.s) to assemble
        Object,   ///< object file (.o) to link
        Library,  ///< library to link against
    };
    /// Number of FileType buckets; keep in sync with FileType.
    static constexpr std::size_t FileTypeCount = 4;

    /** The kind of artifact the compiler produces. */
    enum class OutputType : std::uint8_t {
        Executable, ///< linked native executable (default)
        Object,     ///< native object file
        Assembly,   ///< native assembly listing
        LlvmIr,     ///< textual LLVM IR
    };

    /** Optimisation level, mirroring the familiar -O command-line flags. */
    enum class OptimizationLevel : std::uint8_t {
        O0, ///< no optimisation (default)
        O1, ///< basic optimisation
        O2, ///< full optimisation
        O3, ///< aggressive optimisation
        Os, ///< optimise for size
        Oz, ///< optimise aggressively for size
    };

    /**
     * Target architecture family. Combined with Bitness this picks the concrete
     * LLVM target (e.g. Arm + Bits64 → aarch64, X86 + Bits32 → i386). Default
     * lets the compiler choose the host architecture.
     */
    enum class Arch : std::uint8_t {
        Default, ///< choose the host architecture
        X86,     ///< Intel / AMD x86 (i386 / x86-64)
        Arm,     ///< ARM (arm / aarch64)
        RiscV,   ///< RISC-V (riscv32 / riscv64)
        PowerPc, ///< PowerPC (ppc / ppc64)
        Mips,    ///< MIPS (mips / mips64)
        Wasm,    ///< WebAssembly (wasm32 / wasm64)
        C,       ///< portable C source output
    };

    /** Target pointer width / data model. Default follows the host. */
    enum class Bitness : std::uint8_t {
        Default, ///< follow the host pointer width
        Bits32,  ///< 32-bit
        Bits64,  ///< 64-bit
    };

    /** Target operating-system platform. Default follows the host. */
    enum class Platform : std::uint8_t {
        Default, ///< follow the host platform
        Linux,   ///< Linux
        Windows, ///< Windows
        MacOS,   ///< macOS / Darwin
    };

    /**
     * Resolve derived configuration in place: make the working directory
     * absolute (defaulting to the current directory) and, when no output path
     * was given, derive one from the first input and the output kind. Call once
     * after populating the options and before handing them to the driver;
     * thereafter the options can be treated as const.
     */
    void finalize();

    // -------------------------------------------------------------------------
    // Mutators
    // -------------------------------------------------------------------------

    /** Append an input file to the bucket for its type. */
    void addFile(const FileType type, const llvm::StringRef path) {
        m_files.at(static_cast<std::size_t>(type)).emplace_back(path);
    }

    /** Append an input file, choosing its bucket from the path extension. */
    void addFile(llvm::StringRef path);

    /** Append a directory to the include search hierarchy (highest precedence first). */
    void addIncludePath(const llvm::StringRef path) { m_includePaths.emplace_back(path); }

    /** Set the explicit output path; empty means "derive a default". */
    void setOutputPath(const llvm::StringRef path) { m_outputPath = path; }

    /** Set the base directory for relative paths; resolved to absolute by @ref finalize. */
    void setWorkingDirectory(const llvm::StringRef path) { m_workingDirectory = path; }

    /** Set the path to the compiler itself (used to locate bundled resources). */
    void setCompilerPath(const llvm::StringRef path) { m_compilerPath = path; }

    /** Set the toolchain directory holding the LLVM binaries (linker, opt, ...). */
    void setToolchainPath(const llvm::StringRef path) { m_toolchainPath = path; }

    /** Select the kind of artifact to emit. */
    void setOutputType(const OutputType type) { m_outputType = type; }

    /** Select the optimisation level. */
    void setOptimizationLevel(const OptimizationLevel level) { m_optimizationLevel = level; }

    /** Select the target architecture (Arch::Default follows the host). */
    void setArch(const Arch arch) { m_arch = arch; }

    /** Select the target pointer width (Bitness::Default follows the host). */
    void setBitness(const Bitness bitness) { m_bitness = bitness; }

    /** Select the target platform (Platform::Default follows the host). */
    void setPlatform(const Platform platform) { m_platform = platform; }

    /** Toggle emission of debug information. */
    void setDebugInfo(const bool enable) { m_debugInfo = enable; }

    /** Toggle dumping the AST (for debugging). */
    void setDumpAst(const bool enable) { m_dumpAst = enable; }

    /** Toggle dumping the lbc IR (for debugging). */
    void setDumpIr(const bool enable) { m_dumpIr = enable; }

    /** Toggle dumping the options as a command line (for debugging). */
    void setDumpConfig(const bool enable) { m_dumpConfig = enable; }

    /** Toggle verbose output. */
    void setVerbose(const bool enable) { m_verbose = enable; }

    // -------------------------------------------------------------------------
    // Observers
    // -------------------------------------------------------------------------

    [[nodiscard]] auto getFiles(const FileType type) const -> llvm::ArrayRef<std::string> {
        return m_files.at(static_cast<std::size_t>(type));
    }
    [[nodiscard]] auto getIncludePaths() const -> llvm::ArrayRef<std::string> { return m_includePaths; }
    [[nodiscard]] auto getOutputPath() const -> llvm::StringRef { return m_outputPath; }
    [[nodiscard]] auto getWorkingDirectory() const -> llvm::StringRef { return m_workingDirectory; }
    [[nodiscard]] auto getCompilerPath() const -> llvm::StringRef { return m_compilerPath; }
    [[nodiscard]] auto getToolchainPath() const -> llvm::StringRef { return m_toolchainPath; }
    [[nodiscard]] auto getOutputType() const -> OutputType { return m_outputType; }
    [[nodiscard]] auto getOptimizationLevel() const -> OptimizationLevel { return m_optimizationLevel; }
    /** The `-O` flag for the selected optimisation level, e.g. "-O2". */
    [[nodiscard]] auto getOptimizationFlag() const -> llvm::StringRef;
    [[nodiscard]] auto getArch() const -> Arch { return m_arch; }
    [[nodiscard]] auto getBitness() const -> Bitness { return m_bitness; }
    [[nodiscard]] auto getPlatform() const -> Platform { return m_platform; }
    [[nodiscard]] auto hasDebugInfo() const -> bool { return m_debugInfo; }
    [[nodiscard]] auto isDumpAst() const -> bool { return m_dumpAst; }
    [[nodiscard]] auto isDumpIr() const -> bool { return m_dumpIr; }
    [[nodiscard]] auto isDumpConfig() const -> bool { return m_dumpConfig; }
    [[nodiscard]] auto isVerbose() const -> bool { return m_verbose; }

    /** Render the options as an equivalent command-line string (for debugging). */
    [[nodiscard]] auto toCommandLine() const -> std::string;

private:
    /** Derive the output path from the first input and the output kind (used by @ref finalize). */
    [[nodiscard]] auto deriveDefaultOutput() const -> std::string;

    std::array<std::vector<std::string>, FileTypeCount> m_files;   ///< input files bucketed by FileType
    std::vector<std::string> m_includePaths;                       ///< include search hierarchy, highest precedence first
    std::string m_outputPath;                                      ///< explicit output path, empty if defaulted
    std::string m_workingDirectory;                                ///< base for relative paths, empty if CWD
    std::string m_compilerPath;                                    ///< path to the lbc compiler itself
    std::string m_toolchainPath;                                   ///< dir holding the LLVM toolchain binaries
    Arch m_arch = Arch::Default;                                   ///< target architecture, host if Default
    Bitness m_bitness = Bitness::Default;                          ///< target pointer width, host if Default
    Platform m_platform = Platform::Default;                       ///< target platform, host if Default
    OutputType m_outputType = OutputType::Executable;              ///< artifact to produce
    OptimizationLevel m_optimizationLevel = OptimizationLevel::O0; ///< optimisation level
    bool m_debugInfo = false;                                      ///< emit debug information
    bool m_dumpAst = false;                                        ///< dump the AST for debugging
    bool m_dumpIr = false;                                         ///< dump the lbc IR for debugging
    bool m_dumpConfig = false;                                     ///< dump the options as a command line
    bool m_verbose = false;                                        ///< verbose diagnostics
};

} // namespace lbc
