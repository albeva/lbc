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
auto addTestEnvironment(auto* ptr) {
    testing::AddGlobalTestEnvironment(ptr);
    return ptr;
}

struct CompileEnvironment : ::testing::Environment {
    CompileEnvironment() = default;
    std::stringstream stdoutput{};
    lbc::CompileOptions options{};
    lbc::Context context{ options };
};

CompileEnvironment* env = addTestEnvironment(new CompileEnvironment{});

// proxy printf
int capturePrintF(const char* format, ...) {
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wformat-nonliteral"

    va_list args;
    va_start(args, format);

    // figure out the length
    int length = std::vsnprintf(nullptr, 0, format, args);
    if (length <= 0) {
        return length;
    }

    auto size = static_cast<std::size_t>(length + 1); // NOLINT(*-misplaced-widening-cast)

    // printf to a buffer
    char* buf = new char[size];
    std::vsnprintf(buf, size, format, args);
    env->stdoutput << buf;
    delete[] buf;

    // done
    va_end(args);
    return length;

    #pragma clang diagnostic pop
}

// proxy puts
int capturePuts(const char* str) {
    env->stdoutput << str;
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
    auto workingPath = getBasePath();

    // reset the shared context
    env->context.reset();

    // Options
    env->options.addInputFile(GetParam());
    env->options.setOptimizationLevel(lbc::CompileOptions::OptimizationLevel::O0);
    env->options.setCompilationTarget(lbc::CompileOptions::CompilationTarget::JIT);
    env->options.setWorkingDir(workingPath);

    // when targeting windows, compiler executable has .exe file extension
    std::string binary = "lbc";
    if (env->context.getTriple().isOSWindows()) {
        binary += ".exe";
    }
    auto compilerPath = canonical(workingPath / "../bin" / binary);
    env->options.setCompilerPath(compilerPath);

    // redirect printf and puts
    exitOnErr(env->context.getJIT().define("printf", &capturePrintF, llvm::JITSymbolFlags::Callable));
    exitOnErr(env->context.getJIT().define("puts", &capturePuts, llvm::JITSymbolFlags::Callable));

    // The driver
    m_driver = std::make_unique<lbc::Driver>(env->context);
}

void CompilerBase::TearDown() {
    env->stdoutput = std::stringstream{};
    m_driver.reset();
}

std::string CompilerBase::run() {
    m_driver->drive();
    return llvm::StringRef{ env->stdoutput.str() }.trim().str();
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
