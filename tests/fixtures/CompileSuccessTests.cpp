//
// Created by Albert Varaksin on 17/06/2026.
//
#include "Utils/CompilerBase.hpp"

namespace {
struct Success : CompilerBase {};

TEST_P(Success, ProducesExpectedOutput) {
    const auto output = run();
    EXPECT_FALSE(failed()) << "expected compilation to succeed";
    EXPECT_EQ(output, expected());
}

INSTANTIATE_TEST_SUITE_P(
    Compile,
    Success,
    testing::ValuesIn(CompilerBase::enumerate("fixtures/success")),
    [](const testing::TestParamInfo<std::filesystem::path>& info) {
        std::string name = info.param.stem().string();
        for (char& ch : name) {
            if (std::isalnum(static_cast<unsigned char>(ch)) == 0) {
                ch = '_';
            }
        }
        return name;
    }
);
} // namespace
