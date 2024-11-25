//
// Created by Albert Varaksin on 02/05/2024.
//
#pragma once
#include <gtest/gtest.h>

namespace lbc {
struct CompileOptions;
class Context;
class Driver;
} // namespace lbc

struct CompilerBase : testing::TestWithParam<std::filesystem::path> {
    CompilerBase();
    ~CompilerBase() override;

    void SetUp() override;
    void TearDown() override;

    std::string expected(bool lookForFile = false);

    std::string run();

    static inline std::filesystem::path getBasePath() {
        return std::filesystem::path{ __FILE__ }.parent_path().parent_path();
    }

    static std::vector<std::filesystem::path> enumerate(const std::filesystem::path& base);

    llvm::ExitOnError exitOnErr{};

private:
    std::unique_ptr<lbc::Driver> m_driver;
};
