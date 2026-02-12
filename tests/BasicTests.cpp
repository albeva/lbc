#include <string>
#include <gtest/gtest.h>
#include "Utilities/Sample.hpp"
using namespace lbc::utils;

TEST(BasicTests, IsTrue) {
    EXPECT_EQ(Sample::getMessage(), "Hello World!");
}
