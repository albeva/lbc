//
// Created by Albert Varaksin on 17/06/2026.
//
#include "Utils/CompilerBase.hpp"

namespace {
struct Fail : CompilerBase {};

TEST_P(Fail, ReportsExpectedError) {
    const auto output = compile();
    EXPECT_TRUE(failed()) << "expected compilation to fail";
    EXPECT_EQ(output, expected());
}

INSTANTIATE_TEST_SUITE_P(
    Compile,
    Fail,
    testing::ValuesIn(CompilerBase::enumerate("fixtures/failures")),
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
