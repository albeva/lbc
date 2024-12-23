//
// Created by Albert Varaksin on 12/06/2021.
//
#include "CompileOptions.hpp"
using namespace lbc;

namespace {
using TypeExpPair = std::pair<std::string_view, CompileOptions::FileType>;
constexpr std::array fileTypeExtMap = {
    TypeExpPair { ".bas", CompileOptions::FileType::Source },
    TypeExpPair { ".s", CompileOptions::FileType::Assembly },
    TypeExpPair { ".o", CompileOptions::FileType::Object },
    TypeExpPair { ".ll", CompileOptions::FileType::LLVMIr },
    TypeExpPair { ".bc", CompileOptions::FileType::BitCode }
};
} // namespace

void CompileOptions::reset() {
    m_logLevel = LogLevel::Silent;
    m_outputType = OutputType::Native;
    m_compilationTarget = CompilationTarget::Executable;
    m_compilationMode = CompilationMode::Bit64;
    m_optimizationLevel = OptimizationLevel::O2;
    m_implicitMain = true;
    m_isDebug = false;
    m_mainPath.reset();
    m_inputFiles.clear();
    m_outputPath.clear();
    m_toolchainDir.clear();
    m_compilerPath.clear();
    m_workingDir.clear();
}

auto CompileOptions::getFileExt(FileType type) -> std::string_view {
    const auto it = std::ranges::find(fileTypeExtMap, type, &TypeExpPair::second);
    if (it != fileTypeExtMap.end()) {
        return it->first;
    }
    fatalError("unknown file type");
}

auto CompileOptions::getFileType(const fs::path& path) -> CompileOptions::FileType {
    const auto it = std::ranges::find(fileTypeExtMap, path.extension(), &TypeExpPair::first);
    if (it != fileTypeExtMap.end()) {
        return it->second;
    }
    // default to source
    return FileType::Source;
}

void CompileOptions::validate() const {
    auto count = getInputCount();

    if (count == 0) {
        fatalError("no input.");
    }

    if (count > 1 && !isTargetLinkable() && !m_outputPath.empty()) {
        fatalError("cannot specify -o when generating multiple output files.");
    }

    if (m_outputType == OutputType::LLVM && isTargetNative()) {
        fatalError("flag -emit-llvm must be combined with -S or -c");
    }

    // .s > `.o`
    if (!getInputFiles(FileType::Assembly).empty()) {
        if (m_outputType == OutputType::LLVM) {
            fatalError("Cannot emit llvm from native assembly");
        }
        if (m_compilationTarget == CompilationTarget::Assembly) {
            fatalError("Invalid output: assembly to assembly");
        }
    }

    // .o > only native linkable target
    if (!getInputFiles(FileType::Object).empty()) {
        if (m_outputType == OutputType::LLVM) {
            fatalError("Cannot emit llvm from native objects");
        }
        if (!isTargetLinkable()) {
            fatalError(".o files can only be added to a linkable target");
        }
    }

    // .ll > everything
    if (!getInputFiles(FileType::LLVMIr).empty()) {
        if (m_outputType == OutputType::LLVM && m_compilationTarget == CompilationTarget::Assembly) {
            fatalError("Invalid output: llvm ir to llvm ir");
        }
    }

    // .bc > everything
    if (!getInputFiles(FileType::BitCode).empty()) {
        if (m_outputType == OutputType::LLVM && m_compilationTarget == CompilationTarget::Object) {
            fatalError("Invalid output: bitcode to bitcode");
        }
    }
}

void CompileOptions::setOutputType(CompileOptions::OutputType outputType) {
    m_outputType = outputType;
    if (m_compilationTarget == CompilationTarget::Executable) {
        setCompilationTarget(CompilationTarget::Assembly);
    }
}

void CompileOptions::setMainFile(const fs::path& file) {
    if (file.extension() != getFileExt(FileType::Source)) {
        fatalError("main file must have '"_t + getFileExt(FileType::Source) + "' extension");
    }
    m_mainPath = file;
    m_implicitMain = true;
    addInputFile(file);
}

void CompileOptions::addInputFile(const fs::path& path) {
    auto type = getFileType(path);
    m_inputFiles[type].emplace_back(path);
}

auto CompileOptions::getInputCount() const -> size_t {
    return std::accumulate(m_inputFiles.begin(), m_inputFiles.end(), size_t {}, [](auto cnt, const auto& vec) {
        return cnt + vec.second.size();
    });
}

void CompileOptions::setWorkingDir(const fs::path& path) {
    if (!path.is_absolute()) {
        fatalError("Working dir not a full path");
    }

    if (!fs::exists(path)) {
        fatalError("Working dir does not exist");
    }

    if (!fs::is_directory(path)) {
        fatalError("Working dir must point to a directory");
    }

    m_workingDir = path;
}

void CompileOptions::setOutputPath(const fs::path& path) {
    if (path.is_absolute()) {
        m_outputPath = path;
    } else {
        m_outputPath = fs::absolute(m_workingDir / path);
    }

    if (fs::exists(m_outputPath) && fs::is_directory(m_outputPath)) {
        fatalError("Output path points to existing directory");
    }
}

void CompileOptions::setCompilerPath(const fs::path& path) {
    m_compilerPath = path;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    if (m_toolchainDir.empty()) {
        auto toolchainDir = getCompilerDir() / "toolchain" / "win64";
        if (fs::exists(toolchainDir)) {
            m_toolchainDir = toolchainDir;
        }
    }
#endif
}

bool CompileOptions::isMainFile(const fs::path& file) const { // NOLINT
    if (!m_implicitMain) {
        return false;
    }

    if (auto main = m_mainPath) {
        if (resolveFilePath(*main) == file) {
            return true;
        }
    }

    const auto& sources = getInputFiles(FileType::Source);
    if (sources.empty()) {
        return false;
    }

    return resolveFilePath(sources.front()) == file;
}

auto CompileOptions::resolveOutputPath(const fs::path& path, std::string_view ext) const -> fs::path {
    if (!fs::exists(path)) {
        fatalError("File '"_t + path.string() + "' not found");
    }
    if (!path.is_absolute()) {
        fatalError("Path '"_t + path.string() + "' is not absolute");
    }
    if (fs::is_directory(path)) {
        fatalError("Path '"_t + path.string() + "' is not a file");
    }

    if (m_outputPath.empty()) {
        fs::path output { path };
        output.replace_extension(ext);
        return output;
    }

    fatalError("output path handling is not implemented");
}

auto CompileOptions::resolveFilePath(const fs::path& path) const -> fs::path {
    if (path.is_absolute()) {
        if (validateFile(path)) {
            return path;
        }
    } else if (auto relToWorkingDir = fs::absolute(m_workingDir / path); validateFile(relToWorkingDir)) {
        return relToWorkingDir;
    } else if (auto relToCompiler = fs::absolute(m_compilerPath / path); validateFile(relToCompiler)) {
        return relToCompiler;
    }

    fatalError("File '"_t + path.string() + "' not found");
}

[[nodiscard]] auto CompileOptions::validateFile(const fs::path& path) -> bool {
    if (!fs::exists(path)) {
        return false;
    }

    if (!fs::is_regular_file(path)) {
        fatalError("File '"_t + path.string() + "' is not regular");
    }

    return true;
}
