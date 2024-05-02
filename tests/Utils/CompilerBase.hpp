//
// Created by Albert Varaksin on 02/05/2024.
//
#pragma once
#include <fstream>
#include <gtest/gtest.h>

namespace lbc {
class CompileOptions;
class Context;
class Driver;
}

struct CompilerBase : testing::TestWithParam<std::filesystem::path> {
    CompilerBase();
    ~CompilerBase() override;

    void SetUp() override;

    std::string expected(bool lookForFile = false);

    std::string run();

    static inline std::filesystem::path getBasePath() {
        return std::filesystem::path{ __FILE__ }.parent_path().parent_path();
    }

    static auto enumerate(const std::filesystem::path& base) -> std::vector<std::filesystem::path> {
        std::vector<std::filesystem::path> paths;
        std::ranges::for_each(std::filesystem::directory_iterator{getBasePath() / base },
            [&](const auto& dir_entry) {
                paths.push_back(dir_entry);
            });
        return paths;
    }

    llvm::ExitOnError exitOnErr{};

private:
    std::unique_ptr<lbc::CompileOptions> m_options;
    std::unique_ptr<lbc::Context> m_ctx;
    std::unique_ptr<lbc::Driver> m_driver;
};
