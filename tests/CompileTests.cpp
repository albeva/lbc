//
// Created by Albert Varaksin on 01/04/2024.
//
#include "Driver/CompileOptions.hpp"
#include "Driver/Context.hpp"
#include "Driver/Driver.hpp"
#include <fstream>
#include <gtest/gtest.h>
namespace {

struct CompilerBase : testing::TestWithParam<std::filesystem::path> {
    void SetUp() override {
        auto workingPath = std::filesystem::current_path();
        auto compilerPath = canonical(workingPath / "../bin/lbc");
        auto source = GetParam();

        m_binary = workingPath / source.stem();

        // Options
        m_options = std::make_unique<lbc::CompileOptions>();
        m_options->addInputFile(source);
        m_options->setOptimizationLevel(lbc::CompileOptions::OptimizationLevel::O0);
        m_options->setWorkingDir(workingPath);
        m_options->setCompilerPath(compilerPath);
        m_options->setOutputPath(m_binary);

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
        // Compile
        m_driver->drive();

        // Args
        std::vector<llvm::StringRef> args;
        std::string binary = m_binary.string();
        args.emplace_back(binary);

        // Output
        std::string out = binary + ".out";
        llvm::ArrayRef<std::optional<llvm::StringRef>> redirects = { std::nullopt, out, std::nullopt };

        // Execute
        EXPECT_EQ(llvm::sys::ExecuteAndWait(binary, args, std::nullopt, redirects), EXIT_SUCCESS);

        // Read the output
        std::ifstream file{ out, std::ios::in };
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

        // clean up
        std::filesystem::remove(binary);
        std::filesystem::remove(out);

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
    std::filesystem::path m_binary{};
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
