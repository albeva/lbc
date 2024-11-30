//
// Created by Albert Varaksin on 02/05/2024.
//
#include "CompilerBase.hpp"
#include "Driver/CompileOptions.hpp"
#include "Driver/Context.hpp"
#include "Driver/Driver.hpp"
#include "Driver/JIT.hpp"
#include <cstdarg>
#include <fstream>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"

namespace {
auto addTestEnvironment(auto* ptr) {
    testing::AddGlobalTestEnvironment(ptr);
    return ptr;
}

struct CompileEnvironment : ::testing::Environment {
    CompileEnvironment() = default;
    std::stringstream stdoutput;
    lbc::CompileOptions options {};
    lbc::Context context { options };
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables, cppcoreguidelines-owning-memory)
CompileEnvironment* env = addTestEnvironment(new CompileEnvironment {});

// proxy printf
// NOLINTNEXTLINE(cert-dcl50-cpp)
auto capturePrintF(const char* format, ...) -> int {
    // NOLINTBEGIN(
    //     cppcoreguidelines-pro-type-vararg,
    //     cert-err33-c,
    //     hicpp-vararg
    // )

    va_list args = nullptr;
    va_start(args, format);

    // figure out the length
    const int length = std::vsnprintf(nullptr, 0, format, args);
    if (length <= 0) {
        return length;
    }

    auto size = static_cast<std::size_t>(length + 1); // NOLINT(*-misplaced-widening-cast)

    // printf to a buffer
    std::vector<char> buf(size);
    std::vsnprintf(buf.data(), size, format, args);
    env->stdoutput << buf.data();

    // done
    va_end(args);
    return length;

    // NOLINTEND(
    //     cppcoreguidelines-pro-type-vararg,
    //     cert-err33-c,
    //     hicpp-vararg
    // )
}

// proxy puts
auto capturePuts(const char* str) -> int {
    env->stdoutput << str;
    return static_cast<int>(strlen(str));
}
} // namespace

auto CompilerBase::enumerate(const std::filesystem::path& base) -> std::vector<std::filesystem::path> {
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
    auto compilerPath = canonical(workingPath / ("../bin/" + binary));
    env->options.setCompilerPath(compilerPath);

    // redirect printf and puts
    exitOnErr(env->context.getJIT().define("printf", &capturePrintF, llvm::JITSymbolFlags::Callable));
    exitOnErr(env->context.getJIT().define("puts", &capturePuts, llvm::JITSymbolFlags::Callable));

    // The driver
    m_driver = std::make_unique<lbc::Driver>(env->context);
}

void CompilerBase::TearDown() {
    env->stdoutput = std::stringstream {};
    m_driver.reset();
}

auto CompilerBase::run() -> std::string {
    m_driver->drive();
    return llvm::StringRef { env->stdoutput.str() }.trim().str();
}

auto CompilerBase::expected(const bool lookForFile) const -> std::string {
    (void)this;
    static constexpr std::string_view prefix = "'' CHECK: ";
    static constexpr std::string_view fileKey = "__FILE__";

    std::string checks {};
    std::ifstream file { GetParam(), std::ios::in };

    std::string line;
    bool first = true;
    while (std::getline(file, line)) {
        if (!line.starts_with(prefix)) {
            continue;
        }

        if (first) {
            first = false;
        } else {
            checks += '\n';
        }

        auto match = line.substr(prefix.length());
        if (lookForFile) {
            if (const size_t pos = match.find(fileKey); pos != std::string::npos) {
                match.replace(pos, fileKey.length(), GetParam().string());
            }
        }
        checks += match;
    }

    return checks;
}
