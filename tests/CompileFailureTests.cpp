//
// Created by Albert Varaksin on 01/04/2024.
//
#include "Utils/CompilerBase.hpp"

struct Fail : CompilerBase { };

TEST_P(Fail, error) {
    auto msg = expected(true);
    msg += '\n';
    EXPECT_EXIT(run(), ::testing::ExitedWithCode(EXIT_FAILURE), ::testing::Eq(msg));
}

INSTANTIATE_TEST_SUITE_P(
    Compile,
    Fail,
    testing::ValuesIn(CompilerBase::enumerate("failures/"))
);
