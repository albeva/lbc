//
// Created by Albert Varaksin on 01/05/2024.
//
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "Driver/CompileOptions.hpp"
#include "Driver/CmdLineParser.hpp"
#include "Driver/Driver.hpp"
#include "Driver/TempFileCache.hpp"
using namespace lbc;
namespace fs = std::filesystem;

class CompileOptionsTests : public ::testing::Test {
protected:
    lbc::CompileOptions options{};

    void TearDown() override {
        Test::TearDown();
        TempFileCache::removeTemporaryFiles();
    }

    static void touch(const fs::path& path) {
        std::ofstream outfile(path, std::ofstream::out);
        outfile.flush();
        outfile.close();
    }
};

TEST_F(CompileOptionsTests, SetMainFile) {
    fs::path mainFile = "/path/to/main/file.bas";
    options.setMainFile(mainFile);
    ASSERT_EQ(options.getMainFile().value(), mainFile);
    ASSERT_TRUE(options.getImplicitMain());
}

TEST_F(CompileOptionsTests, SetOutputPath) {
    fs::path outputPath = "/path/to/output";
    options.setOutputPath(outputPath);
    ASSERT_EQ(options.getOutputPath(), outputPath);
}

TEST_F(CompileOptionsTests, SetCompilerPath) {
    fs::path compilerPath = "/path/to/compiler";
    options.setCompilerPath(compilerPath);
    ASSERT_EQ(options.getCompilerPath(), compilerPath);
}

TEST_F(CompileOptionsTests, SetWorkingDir) {
    auto workingDir = fs::path(__FILE__).parent_path();
    options.setWorkingDir(workingDir);
    ASSERT_EQ(options.getWorkingDir(), workingDir);
}

TEST_F(CompileOptionsTests, AddInputFile) {
    // Add different types of files
    options.addInputFile("test1.lbc");
    options.addInputFile("test2.ll");
    options.addInputFile("test3.bc");
    options.addInputFile("test4.s");
    options.addInputFile("test5.o");

    // Check if the files are properly categorized
    const auto& sourceFiles = options.getInputFiles(lbc::CompileOptions::FileType::Source);
    ASSERT_EQ(sourceFiles.size(), 1);
    ASSERT_EQ(sourceFiles[0], "test1.lbc");

    const auto& llFiles = options.getInputFiles(lbc::CompileOptions::FileType::LLVMIr);
    ASSERT_EQ(llFiles.size(), 1);
    ASSERT_EQ(llFiles[0], "test2.ll");

    const auto& bcFiles = options.getInputFiles(lbc::CompileOptions::FileType::BitCode);
    ASSERT_EQ(bcFiles.size(), 1);
    ASSERT_EQ(bcFiles[0], "test3.bc");

    const auto& assemblyFiles = options.getInputFiles(lbc::CompileOptions::FileType::Assembly);
    ASSERT_EQ(assemblyFiles.size(), 1);
    ASSERT_EQ(assemblyFiles[0], "test4.s");

    const auto& objectFiles = options.getInputFiles(lbc::CompileOptions::FileType::Object);
    ASSERT_EQ(objectFiles.size(), 1);
    ASSERT_EQ(objectFiles[0], "test5.o");
}

TEST_F(CompileOptionsTests, ResolveOutputPathExists) {
    fs::path existingPath = TempFileCache::createUniquePath("test.bas");;
    std::string_view ext = ".txt";

    touch(existingPath);

    fs::path result = options.resolveOutputPath(existingPath, ext);
    ASSERT_EQ(existingPath.replace_extension(ext), result);
}

TEST_F(CompileOptionsTests, ResolveOutputPathIsDirectory) {
    fs::path dirPath = fs::current_path();
    std::string_view ext = ".txt";
    EXPECT_EXIT((void)options.resolveOutputPath(dirPath, ext), ::testing::ExitedWithCode(EXIT_FAILURE),
        ::testing::Eq("lbc: error: Path '" + dirPath.string() + "' is not a file\n"));
}

TEST_F(CompileOptionsTests, ResolveOutputPathIsNotDirectory) {
    fs::path filePath = "/path/to/nonexistent/file";
    std::string_view ext = ".txt";

    EXPECT_EXIT((void)options.resolveOutputPath(filePath, ext), ::testing::ExitedWithCode(EXIT_FAILURE),
        ::testing::Eq("lbc: error: File '" + filePath.string() + "' not found\n"));
}

TEST_F(CompileOptionsTests, ResolveFilePathAbsolute) {
    fs::path absolutePath = TempFileCache::createUniquePath("test.bas");
    touch(absolutePath);
    fs::path result = options.resolveFilePath(absolutePath);
    ASSERT_EQ(absolutePath, result);
}