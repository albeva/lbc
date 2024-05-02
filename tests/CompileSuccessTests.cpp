//
// Created by Albert Varaksin on 01/04/2024.
//
#include "Utils/CompilerBase.hpp"

struct Success: CompilerBase {};

TEST_P(Success, run) {
    EXPECT_EQ(expected(), run());
}

INSTANTIATE_TEST_SUITE_P(
    Compile,
    Success,
    testing::ValuesIn(CompilerBase::enumerate("success/")));
