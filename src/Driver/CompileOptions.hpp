//
// Created by Albert Varaksin on 12/06/2021.
//
#pragma once
#include "pch.hpp"

namespace lbc {

class CompileOptions final {
public:
    enum class CompilationTarget {
        Executable,
        Object,
        Assembly,
        JIT
    };

    enum class OutputType {
        Native,
        LLVM
    };

    enum class OptimizationLevel {
        O0,
        OS,
        O1,
        O2,
        O3
    };

    enum class FileType {
        Source,   // anything, but mostly .bas
        Assembly, // .s
        Object,   // .o
        LLVMIr,   // .ll
        BitCode   // .bc
    };
    static constexpr size_t FILETYPE_COUNT = 5;
    [[nodiscard]] static std::string getFileExt(FileType type);

public:
    NO_COPY_AND_MOVE(CompileOptions)
    CompileOptions() = default;
    ~CompileOptions() = default;

    void validate() const;

public:
    [[nodiscard]] CompilationTarget getCompilationTarget() const { return m_compilationTarget; }
    void setCompilationTarget(CompilationTarget target) { m_compilationTarget = target; }

    [[nodiscard]] bool is64Bit() const { return m_is64bit; }
    void set64Bit() { m_is64bit = true; }
    [[nodiscard]] bool is32Bit() const { return !m_is64bit; }
    void set32Bit() { m_is64bit = false; }

    [[nodiscard]] OutputType getOutputType() const { return m_outputType; }
    void setOutputType(OutputType outputType) {
        m_outputType = outputType;
        if (m_compilationTarget == CompilationTarget::Executable) {
            setCompilationTarget(CompilationTarget::Assembly);
        }
    }

    [[nodiscard]] bool getDumpAst() const { return m_astDump; }
    void setDumpAst(bool dump) { m_astDump = dump; }

    [[nodiscard]] bool getDumpCode() const { return m_codeDump; }
    void setDumpCode(bool dump) { m_codeDump = dump; }

    [[nodiscard]] OptimizationLevel getOptimizationLevel() const { return m_optimizationLevel; }
    void setOptimizationLevel(OptimizationLevel level) { m_optimizationLevel = level; }

    [[nodiscard]] bool isDebugBuild() const { return m_isDebug; }
    void setDebugBuild(bool debug) { m_isDebug = debug; }

    [[nodiscard]] bool isVerbose() const { return m_verbose; }
    void setVerbose(bool verbose) { m_verbose = verbose; }

    [[nodiscard]] bool getImplicitMain() const { return m_implicitMain; }
    void setImplicitMain(bool implicitMain) { m_implicitMain = implicitMain; }

    [[nodiscard]] const std::optional<fs::path>& getMainFile() const { return m_mainPath; }
    void setMainFile(const fs::path& file);

    [[nodiscard]] const std::vector<fs::path>& getInputFiles(FileType type) const;
    void addInputFile(const fs::path& path);

    [[nodiscard]] const fs::path& getOutputPath() const { return m_outputPath; }
    void setOutputPath(const fs::path& path);

    [[nodiscard]] const fs::path& getToolchainDir() const { return m_toolchainDir; }
    void setToolchainDir(const fs::path& path) { m_toolchainDir = path; }

    [[nodiscard]] const fs::path& getCompilerPath() const { return m_compilerPath; }
    [[nodiscard]] fs::path getCompilerDir() const { return m_compilerPath.parent_path(); }
    void setCompilerPath(const fs::path& path);

    [[nodiscard]] const fs::path& getWorkingDir() const { return m_workingDir; }
    void setWorkingDir(const fs::path& path);

public:
    [[nodiscard]] bool isTargetLinkable() const {
        return m_compilationTarget == CompilationTarget::Executable;
    }

    [[nodiscard]] bool isTargetNative() const {
        return m_compilationTarget == CompilationTarget::Executable;
    }

    [[nodiscard]] bool isOutputLLVMIr() const {
        return m_outputType == OutputType::LLVM && m_compilationTarget == CompilationTarget::Assembly;
    }

public:
    [[nodiscard]] bool isMainFile(const fs::path& file) const;
    [[nodiscard]] fs::path resolveOutputPath(const fs::path& path, const std::string& ext) const;
    [[nodiscard]] fs::path resolveFilePath(const fs::path& path) const;

private:
    [[nodiscard]] size_t getInputCount() const;
    [[nodiscard]] static bool validateFile(const fs::path& path);

    bool m_verbose = false;
    OutputType m_outputType = OutputType::Native;
    CompilationTarget m_compilationTarget = CompilationTarget::Executable;
    bool m_is64bit = true;
    OptimizationLevel m_optimizationLevel = OptimizationLevel::O2;
    bool m_implicitMain = true;
    bool m_isDebug = false;
    bool m_astDump = false;
    bool m_codeDump = false;
    std::optional<fs::path> m_mainPath{};
    std::array<std::vector<fs::path>, FILETYPE_COUNT> m_inputFiles{};
    fs::path m_outputPath{};
    fs::path m_toolchainDir{};
    fs::path m_compilerPath{};
    fs::path m_workingDir{};
};

} // namespace lbc
