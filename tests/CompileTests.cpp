//
// Created by Albert Varaksin on 01/04/2024.
//
#include "Driver/CompileOptions.hpp"
#include "Driver/Context.hpp"
#include "Driver/Driver.hpp"
#include "Driver/JIT.hpp"
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <gtest/gtest.h>
#include <llvm-c/Target.h>
#include <llvm/Support/TargetSelect.h>
namespace {
// capture stdout into stream
thread_local std::stringstream stdoutput = {};

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

llvm::ExitOnError exitOnErr{};

struct CompilerBase : testing::TestWithParam<std::filesystem::path> {
    void SetUp() override {
        auto workingPath = std::filesystem::current_path();

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

    std::string expected() {
        (void)this;
        static constexpr std::string_view prefix{ "'' CHECK: " };

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

            checks += llvm::StringRef(line).substr(prefix.length()).trim();
        }

        return checks;
    }

    std::string reality() {
        stdoutput.clear();
        m_driver->drive();
        return llvm::StringRef{ stdoutput.str() }.trim().str();
    }

    static auto enumerate(const std::filesystem::path& base) -> std::vector<std::filesystem::path> {
        std::vector<std::filesystem::path> paths;
        std::ranges::for_each(std::filesystem::directory_iterator{ base },
            [&](const auto& dir_entry) {
                paths.push_back(dir_entry);
            });
        return paths;
    }

private:
    std::unique_ptr<lbc::CompileOptions> m_options;
    std::unique_ptr<lbc::Context> m_ctx;
    std::unique_ptr<lbc::Driver> m_driver;
};

// Tests

struct CompileSuccess : CompilerBase {};

TEST_P(CompileSuccess, Success) {
    EXPECT_EQ(expected(), reality());
}

INSTANTIATE_TEST_SUITE_P(
    Run,
    CompileSuccess,
    testing::ValuesIn(CompilerBase::enumerate("success/")));
} // namespace
