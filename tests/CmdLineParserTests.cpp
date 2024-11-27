//
// Created by Albert Varaksin on 01/05/2024.
//
#include "Driver/CmdLineParser.hpp"
#include "Driver/CompileOptions.hpp"
#include "gtest/gtest.h"

class CmdLineParserTest : public ::testing::Test {
protected:
    lbc::CompileOptions options{}; // NOLINT
    lbc::CmdLineParser parser{ options }; // NOLINT

    void parse(std::initializer_list<const char*> args) {
        parser.parse(lbc::CmdLineParser::Args(args.begin(), args.size()));
    }
};

#define TEST_FAILURE(Msg, ...) \
    EXPECT_EXIT(parse({ __VA_ARGS__ }), ::testing::ExitedWithCode(EXIT_FAILURE), ::testing::Eq(Msg))

#define TEST_SUCCESS(Msg, ...) \
    EXPECT_EXIT(parse({ __VA_ARGS__ }), ::testing::ExitedWithCode(EXIT_SUCCESS), ::testing::Eq(Msg))


TEST_F(CmdLineParserTest, ParseValidArguments) {
    parse({ "lbc", "-v", "-o", "output.o", "input.bas" });
    auto current = std::filesystem::current_path();
    EXPECT_EQ(options.getLogLevel(), lbc::CompileOptions::LogLevel::Verbose);
    EXPECT_EQ(options.getOutputPath(), current / "output.o");
    EXPECT_EQ(options.getInputFiles(lbc::CompileOptions::FileType::Source).front(), "input.bas");
}

TEST_F(CmdLineParserTest, ParseInvalidArguments) {
    TEST_FAILURE(
        "Unrecognized option -invalid. Use --help for more info\n",
        "lbc",
        "-invalid"
    );
}

TEST_F(CmdLineParserTest, ParseNoArguments) {
    TEST_FAILURE(
        "no input. Use --help for more info\n",
        "lbc"
    );
}

TEST_F(CmdLineParserTest, ParseMissingOutputFile) {
    TEST_FAILURE(
        "output file path missing. Use --help for more info\n",
        "lbc",
        "-o"
    );
}

TEST_F(CmdLineParserTest, ParseMissingMainFile) {
    TEST_FAILURE(
        "file path missing. Use --help for more info\n",
        "lbc",
        "-main"
    );
}

TEST_F(CmdLineParserTest, ParseMissingToolchainPath) {
    TEST_FAILURE(
        "Toolchain path is missing Use --help for more info\n",
        "lbc",
        "--toolchain"
    );
}

TEST_F(CmdLineParserTest, ParseInvalidOptimizationLevel) {
    TEST_FAILURE(
        "Unrecognized option -O4. Use --help for more info\n",
        "lbc",
        "-O4"
    );
}

TEST_F(CmdLineParserTest, ParseInvalidCompilationMode) {
    TEST_FAILURE(
        "Unrecognized option -m128. Use --help for more info\n",
        "lbc",
        "-m128"
    );
}

TEST_F(CmdLineParserTest, ParseHelpOption) {
    TEST_SUCCESS("", "lbc", "--help");
}

TEST_F(CmdLineParserTest, ParseVersionOption) {
    TEST_SUCCESS("", "lbc", "--version");
}
