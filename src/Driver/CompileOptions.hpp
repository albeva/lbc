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
    enum class CompilationTarget : std::uint8_t {
        Executable, ///< The target is an executable file.
        Object, ///< The target is an object file.
        Assembly, ///< The target is an assembly file.
        JIT ///< The target is a Just-In-Time compilation.
    };

    /**
     * @enum OutputType
     * @brief Specifies the type of the output file.
     *
     * This enum is used to determine the type of the output file.
     */
    enum class OutputType : std::uint8_t {
        Native, ///< The output file is a native file.
        LLVM ///< The output file is an LLVM file.
    };

    /**
     * @enum OptimizationLevel
     * @brief Specifies the level of optimization to apply during the compilation process.
     *
     * This enum is used to determine the level of optimization to apply during the compilation process.
     */
    enum class OptimizationLevel : std::uint8_t {
        O0, ///< No optimization.
        OS, ///< Size optimization.
        O1, ///< Level 1 optimization.
        O2, ///< Level 2 optimization.
        O3 ///< Level 3 optimization.
    };

    /**
     * @enum CompilationMode
     * @brief Specifies the mode of the compilation process.
     *
     * This enum is used to determine the mode of the compilation process.
     */
    enum class CompilationMode : std::uint8_t {
        Bit32, ///< The compilation process is in 32-bit mode.
        Bit64 ///< The compilation process is in 64-bit mode.
    };

    /**
     * @enum LogLevel
     * @brief Specifies the level of logging during the compilation process.
     *
     * This enum is used to determine the level of logging during the compilation process.
     */
    enum class LogLevel : std::uint8_t {
        Silent, ///< No logging.
        Verbose, ///< Verbose logging.
        Debug ///< Debug logging.
    };

    /**
     * @enum FileType
     * @brief Specifies the type of the input file.
     *
     * This enum is used to determine the type of the input or output file.
     */
    enum class FileType : std::uint8_t {
        Source, ///< File is a source file.
        Assembly, ///< File is an assembly file.
        Object, ///< File is an object file.
        LLVMIr, ///< File is an LLVM IR file.
        BitCode ///< File is a bitcode file.
    };

    using FilesVector = std::vector<fs::path>;
    using FilesMap = std::unordered_map<FileType, FilesVector>;

    /**
     * Reset the options to their default values.
     */
    void reset();

    /**
     * @brief Get the file extension for a given file type.
     * @param type The file type.
     * @return The file extension as a string view.
     */
    [[nodiscard]] static auto getFileExt(FileType type) -> std::string_view;

    /**
     * @brief Get the file type for a given file path.
     *
     * If the file extension is not recognized, FileType::Source is returned.
     *
     * @param path The file path.
     * @return The file type.
     */
    [[nodiscard]] static auto getFileType(const fs::path& path) -> FileType;

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
    [[nodiscard]] auto getCompilationTarget() const -> CompilationTarget { return m_compilationTarget; }

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
    [[nodiscard]] auto getCompilationMode() const -> CompilationMode {
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
    [[nodiscard]] auto getOutputType() const -> OutputType { return m_outputType; }

    /**
     * @brief Set the output type.
     *
     * @param outputType The new output type.
     */
    void setOutputType(OutputType outputType);

    /**
     * @brief Get the current optimization level.
     *
     * @return The current optimization level.
     */
    [[nodiscard]] auto getOptimizationLevel() const -> OptimizationLevel { return m_optimizationLevel; }

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
    [[nodiscard]] auto isDebugBuild() const -> bool { return m_isDebug; }

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
    [[nodiscard]] auto logVerbose() const -> bool { return m_logLevel == LogLevel::Verbose; }

    /**
     * @brief Check if the log level is debug or verbose.
     *
     * @return True if the log level is debug or verbose, false otherwise.
     */
    [[nodiscard]] auto logDebug() const -> bool { return m_logLevel == LogLevel::Verbose || m_logLevel == LogLevel::Debug; }

    /**
     * @brief Get the current log level.
     *
     * @return The current log level.
     */
    [[nodiscard]] auto getLogLevel() const -> LogLevel { return m_logLevel; }

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
    [[nodiscard]] auto getImplicitMain() const -> bool { return m_implicitMain; }

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
    [[nodiscard]] auto getMainFile() const -> const std::optional<fs::path>& { return m_mainPath; }

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
    [[nodiscard]] auto getInputFiles() const -> const FilesMap& { return m_inputFiles; }

    /**
     * @brief Get the input files of a specific type.
     *
     * @param type The type of the input files to get.
     * @return The input files of the specified type.
     */
    [[nodiscard]] auto getInputFiles(FileType type) const -> const FilesVector& { return m_inputFiles[type]; }

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
    [[nodiscard]] auto getOutputPath() const -> const fs::path& { return m_outputPath; }

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
    [[nodiscard]] auto getToolchainDir() const -> const fs::path& { return m_toolchainDir; }

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
    [[nodiscard]] auto getCompilerPath() const -> const fs::path& { return m_compilerPath; }

    /**
     * @brief Get the compiler directory.
     *
     * @return The compiler directory.
     */
    [[nodiscard]] auto getCompilerDir() const -> fs::path { return m_compilerPath.parent_path(); }

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
    [[nodiscard]] auto getWorkingDir() const -> const fs::path& { return m_workingDir; }

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
    [[nodiscard]] auto isTargetLinkable() const -> bool {
        return m_compilationTarget == CompilationTarget::Executable;
    }

    /**
     * @brief Checks whether the target is native executable.
     *
     * @return true if the target is native, false otherwise.
     */
    [[nodiscard]] auto isTargetNative() const -> bool {
        return m_compilationTarget == CompilationTarget::Executable;
    }

    /**
     * Checks whether the output is LLVM IR.
     *
     * @return true if the output is LLVM IR, false otherwise.
     */
    [[nodiscard]] auto isOutputLLVMIr() const -> bool {
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
    [[nodiscard]] auto isMainFile(const fs::path& file) const -> bool;

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
    [[nodiscard]] auto resolveOutputPath(const fs::path& path, std::string_view ext) const -> fs::path;

    /**
     * @brief Resolve the file path.
     *
     * This function takes a potentially relative file path and returns an absolute path.
     * Path can be relative to working directory or the compiler directory.
     *
     * @param path The path of the file.
     * @return The resolved file path.
     */
    [[nodiscard]] auto resolveFilePath(const fs::path& path) const -> fs::path;

private:
    /**
     * @brief Get the count of input files.
     *
     * @return The count of input files.
     */
    [[nodiscard]] auto getInputCount() const -> size_t;

    /**
     * @brief Validate the given file path.
     *
     * This function checks if the given file path exists and is a regular file.
     *
     * @param path The path of the file to validate.
     * @return True if the file exists and is a regular file, false otherwise.
     */
    [[nodiscard]] static auto validateFile(const fs::path& path) -> bool;

    LogLevel m_logLevel = LogLevel::Silent;
    OutputType m_outputType = OutputType::Native;
    CompilationTarget m_compilationTarget = CompilationTarget::Executable;
    CompilationMode m_compilationMode = CompilationMode::Bit64;
    OptimizationLevel m_optimizationLevel = OptimizationLevel::O2;
    bool m_implicitMain = true;
    bool m_isDebug = false;
    std::optional<fs::path> m_mainPath;
    mutable FilesMap m_inputFiles;
    fs::path m_outputPath;
    fs::path m_toolchainDir;
    fs::path m_compilerPath;
    fs::path m_workingDir;
};

} // namespace lbc
