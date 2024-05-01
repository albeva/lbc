//
// Created by Albert Varaksin on 12/06/2021.
//
#pragma once
#include "pch.hpp"

namespace lbc {

/**
 * @struct CompileOptions
 * @brief Handles the compilation options for the LightBASIC compiler.
 *
 * This class is responsible for managing the compilation options
 * provided to the program. It includes options for the target
 * architecture, output type, optimization level, and more.
 */
struct CompileOptions final {
    /**
     * @enum CompilationTarget
     * @brief Specifies the target of the compilation process.
     *
     * This enum is used to determine the final output of the compilation process.
     */
    enum class CompilationTarget {
        Executable, ///< The target is an executable file.
        Object,     ///< The target is an object file.
        Assembly,   ///< The target is an assembly file.
        JIT         ///< The target is a Just-In-Time compilation.
    };

    /**
     * @enum OutputType
     * @brief Specifies the type of the output file.
     *
     * This enum is used to determine the type of the output file.
     */
    enum class OutputType {
        Native, ///< The output file is a native file.
        LLVM    ///< The output file is an LLVM file.
    };

    /**
     * @enum OptimizationLevel
     * @brief Specifies the level of optimization to apply during the compilation process.
     *
     * This enum is used to determine the level of optimization to apply during the compilation process.
     */
    enum class OptimizationLevel {
        O0, ///< No optimization.
        OS, ///< Size optimization.
        O1, ///< Level 1 optimization.
        O2, ///< Level 2 optimization.
        O3  ///< Level 3 optimization.
    };

    /**
     * @enum CompilationMode
     * @brief Specifies the mode of the compilation process.
     *
     * This enum is used to determine the mode of the compilation process.
     */
    enum class CompilationMode {
        Bit32, ///< The compilation process is in 32-bit mode.
        Bit64  ///< The compilation process is in 64-bit mode.
    };

    /**
     * @enum LogLevel
     * @brief Specifies the level of logging during the compilation process.
     *
     * This enum is used to determine the level of logging during the compilation process.
     */
    enum class LogLevel {
        Silent,  ///< No logging.
        Verbose, ///< Verbose logging.
        Debug    ///< Debug logging.
    };

    /**
     * @enum FileType
     * @brief Specifies the type of the input file.
     *
     * This enum is used to determine the type of the input or output file.
     */
    enum class FileType {
        Source,   ///< File is a source file.
        Assembly, ///< File is an assembly file.
        Object,   ///< File is an object file.
        LLVMIr,   ///< File is an LLVM IR file.
        BitCode   ///< File is a bitcode file.
    };

    using FilesVector = std::vector<fs::path>;
    using FilesMap = std::unordered_map<FileType, FilesVector>;

    /**
     * @brief Get the file extension for a given file type.
     * @param type The file type.
     * @return The file extension as a string view.
     */
    [[nodiscard]] static std::string_view getFileExt(FileType type);

    /**
     * @brief Get the file type for a given file path.
     *
     * If the file extension is not recognized, FileType::Source is returned.
     *
     * @param path The file path.
     * @return The file type.
     */
    [[nodiscard]] static FileType getFileType(const fs::path& path);

    /**
     * @brief Validate the current compilation options.
     *
     * This function checks the current state of the compilation options and ensures they are valid.
     * If the options are not valid, it will terminate the program with an error message.
     *
     * This function should be called after all options have been set and before they are used.
     */
    void validate() const;

    /**
     * @brief Get the current compilation target.
     *
     * @return The current compilation target.
     */
    [[nodiscard]] CompilationTarget getCompilationTarget() const { return m_compilationTarget; }

    /**
     * @brief Set the compilation target.
     *
     * @param target The new compilation target.
     */
    void setCompilationTarget(CompilationTarget target) { m_compilationTarget = target; }

    /**
     * @brief Get the current compilation mode.
     *
     * @return The current compilation mode.
     */
    [[nodiscard]] CompilationMode getCompilationMode() const {
        return m_compilationMode;
    }

    /**
     * @brief Set the compilation mode.
     *
     * @param mode The new compilation mode.
     */
    void setCompilationMode(CompilationMode mode) {
        m_compilationMode = mode;
    }

    /**
     * @brief Get the current output type.
     *
     * @return The current output type.
     */
    [[nodiscard]] OutputType getOutputType() const { return m_outputType; }

    /**
     * @brief Set the output type.
     *
     * @param outputType The new output type.
     */
    void setOutputType(OutputType outputType);

    /**
     * @brief Check if the AST dump is enabled.
     *
     * @return True if the AST dump is enabled, false otherwise.
     */
    [[nodiscard]] bool getDumpAst() const { return m_astDump; }

    /**
     * @brief Enable or disable the AST dump.
     *
     * @param dump True to enable the AST dump, false to disable it.
     */
    void setDumpAst(bool dump) { m_astDump = dump; }

    /**
     * @brief Check if the code dump is enabled.
     *
     * @return True if the code dump is enabled, false otherwise.
     */
    [[nodiscard]] bool getDumpCode() const { return m_codeDump; }

    /**
     * @brief Enable or disable the code dump.
     *
     * @param dump True to enable the code dump, false to disable it.
     */
    void setDumpCode(bool dump) { m_codeDump = dump; }

    /**
     * @brief Get the current optimization level.
     *
     * @return The current optimization level.
     */
    [[nodiscard]] OptimizationLevel getOptimizationLevel() const { return m_optimizationLevel; }

    /**
     * @brief Set the optimization level.
     *
     * @param level The new optimization level.
     */
    void setOptimizationLevel(OptimizationLevel level) { m_optimizationLevel = level; }

    /**
     * @brief Check if the build is a debug build.
     *
     * @return True if the build is a debug build, false otherwise.
     */
    [[nodiscard]] bool isDebugBuild() const { return m_isDebug; }

    /**
     * @brief Set whether the build is a debug build.
     *
     * @param debug True to set the build as a debug build, false otherwise.
     */
    void setDebugBuild(bool debug) { m_isDebug = debug; }

    /**
     * @brief Check if the log level is verbose.
     *
     * @return True if the log level is verbose, false otherwise.
     */
    [[nodiscard]] bool logVerbose() const { return m_logLevel == LogLevel::Verbose; }

    /**
     * @brief Check if the log level is debug or verbose.
     *
     * @return True if the log level is debug or verbose, false otherwise.
     */
    [[nodiscard]] bool logDebug() const { return m_logLevel == LogLevel::Verbose || m_logLevel == LogLevel::Debug; }

    /**
     * @brief Get the current log level.
     *
     * @return The current log level.
     */
    [[nodiscard]] LogLevel getLogLevel() const { return m_logLevel; }

    /**
     * @brief Set the log level.
     *
     * @param level The new log level.
     */
    void setLogLevel(LogLevel level) { m_logLevel = level; }

    /**
     * @brief Check if the main function is implicit.
     *
     * @return True if the main function is implicit, false otherwise.
     */
    [[nodiscard]] bool getImplicitMain() const { return m_implicitMain; }

    /**
     * @brief Set whether the main function is implicit.
     *
     * @param implicitMain True to set the main function as implicit, false otherwise.
     */
    void setImplicitMain(bool implicitMain) { m_implicitMain = implicitMain; }

    /**
     * @brief Get the main file path.
     *
     * @return The main file path.
     */
    [[nodiscard]] const std::optional<fs::path>& getMainFile() const { return m_mainPath; }

    /**
     * @brief Set the main file path.
     *
     * @param file The new main file path.
     */
    void setMainFile(const fs::path& file);

    /**
     * @brief Get the input files.
     *
     * @return The input files mapped by their type.
     */
    [[nodiscard]] const FilesMap& getInputFiles() const { return m_inputFiles; }

    /**
     * @brief Get the input files of a specific type.
     *
     * @param type The type of the input files to get.
     * @return The input files of the specified type.
     */
    [[nodiscard]] const FilesVector& getInputFiles(FileType type) const { return m_inputFiles[type]; }

    /**
     * @brief Add an input file.
     *
     * @param path The path of the input file to add.
     */
    void addInputFile(const fs::path& path);

    /**
     * @brief Get the output path.
     *
     * @return The output path.
     */
    [[nodiscard]] const fs::path& getOutputPath() const { return m_outputPath; }

    /**
     * @brief Set the output path.
     *
     * @param path The new output path.
     */
    void setOutputPath(const fs::path& path);

    /**
     * @brief Get the toolchain directory.
     *
     * @return The toolchain directory.
     */
    [[nodiscard]] const fs::path& getToolchainDir() const { return m_toolchainDir; }

    /**
     * @brief Set the toolchain directory.
     *
     * @param path The new toolchain directory.
     */
    void setToolchainDir(const fs::path& path) { m_toolchainDir = path; }

    /**
     * @brief Get the compiler path.
     *
     * @return The compiler path.
     */
    [[nodiscard]] const fs::path& getCompilerPath() const { return m_compilerPath; }

    /**
     * @brief Get the compiler directory.
     *
     * @return The compiler directory.
     */
    [[nodiscard]] fs::path getCompilerDir() const { return m_compilerPath.parent_path(); }

    /**
     * @brief Set the compiler path.
     *
     * @param path The new compiler path.
     */
    void setCompilerPath(const fs::path& path);

    /**
     * @brief Get the working directory.
     *
     * @return The working directory.
     */
    [[nodiscard]] const fs::path& getWorkingDir() const { return m_workingDir; }

    /**
     * @brief Set the working directory.
     *
     * @param path The new working directory.
     */
    void setWorkingDir(const fs::path& path);

    /**
     * @brief Checks whether the target is linkable.
     *
     * @return true if the target is linkable, false otherwise.
     */
    [[nodiscard]] bool isTargetLinkable() const {
        return m_compilationTarget == CompilationTarget::Executable;
    }

    /**
     * @brief Checks whether the target is native executable.
     *
     * @return true if the target is native, false otherwise.
     */
    [[nodiscard]] bool isTargetNative() const {
        return m_compilationTarget == CompilationTarget::Executable;
    }

    /**
     * Checks whether the output is LLVM IR.
     *
     * @return true if the output is LLVM IR, false otherwise.
     */
    [[nodiscard]] bool isOutputLLVMIr() const {
        return m_outputType == OutputType::LLVM && m_compilationTarget == CompilationTarget::Assembly;
    }

    /**
     * Checks if the given file is the main file.
     *
     * Main file includes the optional implicit or explicit main function
     *
     * @param file The file to check.
     * @return True if the file is the main file, false otherwise.
     */
    [[nodiscard]] bool isMainFile(const fs::path& file) const;

    /**
     * @brief Resolve the output path for a given file.
     *
     * This function takes a file path and an extension, and returns a new path with replaced
     * extension. If the path does not exist, will raise a fatal error.
     *
     * @param path The path of the file.
     * @param ext The extension of the output file.
     * @return The resolved output path.
     */
    [[nodiscard]] fs::path resolveOutputPath(const fs::path& path, std::string_view ext) const;

    /**
     * @brief Resolve the file path.
     *
     * This function takes a potentially relative file path and returns an absolute path.
     * Path can be relative to working directory or the compiler directory.
     *
     * @param path The path of the file.
     * @return The resolved file path.
     */
    [[nodiscard]] fs::path resolveFilePath(const fs::path& path) const;

private:
    /**
     * @brief Get the count of input files.
     *
     * @return The count of input files.
     */
    [[nodiscard]] size_t getInputCount() const;

    /**
     * @brief Validate the given file path.
     *
     * This function checks if the given file path exists and is a regular file.
     *
     * @param path The path of the file to validate.
     * @return True if the file exists and is a regular file, false otherwise.
     */
    [[nodiscard]] static bool validateFile(const fs::path& path);

    LogLevel m_logLevel = LogLevel::Silent;
    OutputType m_outputType = OutputType::Native;
    CompilationTarget m_compilationTarget = CompilationTarget::Executable;
    CompilationMode m_compilationMode = CompilationMode::Bit64;
    OptimizationLevel m_optimizationLevel = OptimizationLevel::O2;
    bool m_implicitMain = true;
    bool m_isDebug = false;
    bool m_astDump = false;
    bool m_codeDump = false;
    std::optional<fs::path> m_mainPath{};
    mutable FilesMap m_inputFiles{};
    fs::path m_outputPath{};
    fs::path m_toolchainDir{};
    fs::path m_compilerPath{};
    fs::path m_workingDir{};
};

} // namespace lbc
