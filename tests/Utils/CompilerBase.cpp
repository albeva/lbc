//
// Created by Albert Varaksin on 02/05/2024.
//
#include "CompilerBase.hpp"
#include "Driver/CompileOptions.hpp"
#include "Driver/Context.hpp"
#include "Driver/Driver.hpp"
#include "Driver/JIT.hpp"
#include <fstream>
#include <cstdarg>

namespace {
// capture stdout into stream
std::stringstream stdoutput = {};

// proxy printf
int capturePrintF(const char* format, ...) {
    va_list args;
    va_start(args, format);

    // figure out the length
    int length = std::vsnprintf(nullptr, 0, format, args);
    if (length <= 0) {
        return length;
    }

    // printf to a buffer
    char* buf = new char[length + 1];
    std::vsnprintf(buf, length + 1, format, args);
    stdoutput << buf;
    delete[] buf;

    // done
    va_end(args);
    return length;
}

// proxy puts
int capturePuts(const char* str) {
    stdoutput << str;
    return static_cast<int>(strlen(str));
}
} // namespace

std::vector<std::filesystem::path> CompilerBase::enumerate(const std::filesystem::path& base) {
    std::vector<std::filesystem::path> paths;
    std::ranges::copy_if(
        std::filesystem::recursive_directory_iterator(getBasePath() / base),
        std::back_inserter(paths),
        [](const auto& entry) {
            return entry.is_regular_file();
        }
    );
    return paths;
}

CompilerBase::CompilerBase() = default;
CompilerBase::~CompilerBase() = default;

void CompilerBase::SetUp() {
    std::cout << "Running test: " << GetParam() << std::endl;
    auto workingPath = getBasePath();

    // Options
    m_options = std::make_unique<lbc::CompileOptions>();
    m_options->addInputFile(GetParam());
    m_options->setOptimizationLevel(lbc::CompileOptions::OptimizationLevel::O0);
    m_options->setCompilationTarget(lbc::CompileOptions::CompilationTarget::JIT);
    m_options->setWorkingDir(workingPath);

    // The context
    m_ctx = std::make_unique<lbc::Context>(*m_options);

    // when targeting windows, compiler executable has .exe file extension
    std::string binary = "lbc";
    if (m_ctx->getTriple().isOSWindows()) {
        binary += ".exe";
    }
    auto compilerPath = canonical(workingPath / ("../bin/" + binary));
    m_options->setCompilerPath(compilerPath);

    // redirect printf
    exitOnErr(m_ctx->getJIT().define("printf", &capturePrintF, llvm::JITSymbolFlags::Callable));
    exitOnErr(m_ctx->getJIT().define("puts", &capturePuts, llvm::JITSymbolFlags::Callable));

    // The driver
    m_driver = std::make_unique<lbc::Driver>(*m_ctx);
}

void CompilerBase::TearDown() {
    std::cout << "Finished test: " << GetParam() << std::endl;

    stdoutput = std::stringstream{};

    m_driver.reset();
    m_ctx.reset();
    m_options.reset();
}

std::string CompilerBase::run() {
    m_driver->drive();
    return llvm::StringRef{ stdoutput.str() }.trim().str();
}

std::string CompilerBase::expected(bool lookForFile) {
    (void)this;
    static constexpr std::string_view prefix = "'' CHECK: ";
    static constexpr std::string_view fileKey = "__FILE__";

    std::string checks{};
    std::ifstream file{ GetParam(), std::ios::in };

    std::string line;
    bool first = true;
    while (std::getline(file, line)) {
        if (!line.starts_with(prefix))
            continue;

        if (first)
            first = false;
        else
            checks += '\n';

        auto match = line.substr(prefix.length());
        if (lookForFile) {
            if (size_t pos = match.find(fileKey); pos != std::string::npos) {
                match.replace(pos, fileKey.length(), GetParam().string());
            }
        }
        checks += match;
    }

    return checks;
}
