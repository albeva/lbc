//
// Created by Albert Varaksin on 01/04/2024.
//
#include "Driver/CompileOptions.hpp"
#include "Driver/Context.hpp"
#include "Driver/Driver.hpp"
#include "Driver/JIT.hpp"
#include "Utils/StdCapture.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm-c/Target.h>
namespace {

struct CompilerBase : testing::TestWithParam<std::filesystem::path> {
    void SetUp() override {
        auto workingPath = std::filesystem::current_path();
        auto compilerPath = canonical(workingPath / "../bin/lbc");

        // Options
        m_options = std::make_unique<lbc::CompileOptions>();
        m_options->addInputFile(GetParam());
        m_options->setOptimizationLevel(lbc::CompileOptions::OptimizationLevel::O0);
        m_options->setCompilationTarget(lbc::CompileOptions::CompilationTarget::JIT);
        m_options->setWorkingDir(workingPath);
        m_options->setCompilerPath(compilerPath);

        // The context
        m_ctx = std::make_unique<lbc::Context>(*m_options);

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
        // Run
        auto capture = lbc::CaptureStd::out();
        m_driver->drive();
        auto out = capture.finish();

        // Read the output
        std::stringstream file{ out, std::ios::in };
        std::string result{};
        std::string line{};
        bool first = true;
        while (std::getline(file, line)) {
            if (first)
                first = false;
            else
                result += '\n';
            result += llvm::StringRef(line).trim();
        }

        return result;
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
