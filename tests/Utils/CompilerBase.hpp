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
    NO_COPY_AND_MOVE(CompilerBase)

    CompilerBase();
    ~CompilerBase() override;

    void SetUp() override;
    void TearDown() override;

    auto expected(bool lookForFile = false) -> std::string;

    auto run() -> std::string;

    static auto getBasePath() -> std::filesystem::path {
        return std::filesystem::path { __FILE__ }.parent_path().parent_path();
    }

    static auto enumerate(const std::filesystem::path& base) -> std::vector<std::filesystem::path>;

private:
    llvm::ExitOnError exitOnErr {};
    std::unique_ptr<lbc::Driver> m_driver;
};
