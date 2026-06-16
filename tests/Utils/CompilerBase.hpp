//
// File-driven compiler test base.
//
// Each fixture is a `.bas` file under `tests/<dir>`; `'' CHECK:` comment lines
// declare the expected compiler output (`__FILE__` expands to the fixture path).
//
#pragma once
#include "pch.hpp"
#include <filesystem>
#include <gtest/gtest.h>
namespace llvm {
class Module;
}
namespace lbc {
class Context;
}

class CompilerBase : public testing::TestWithParam<std::filesystem::path> {
public:
    /** Fixture files under `tests/<dir>`, sorted by path. */
    [[nodiscard]] static auto enumerate(const std::filesystem::path& dir) -> std::vector<std::filesystem::path>;

protected:
    /** Run the front end over the fixture in-process and return the rendered diagnostics. */
    [[nodiscard]] auto compile() -> std::string;

    /** Compile the fixture and JIT-execute it; return the captured stdout (or diagnostics on failure). */
    [[nodiscard]] auto run() -> std::string;

    /** Whether the most recent compile()/run() reported an error. */
    [[nodiscard]] auto failed() const -> bool { return m_failed; }

    /** Expected output assembled from the fixture's `'' CHECK:` lines. */
    [[nodiscard]] auto expected() const -> std::string;

private:
    [[nodiscard]] static auto basePath() -> std::filesystem::path {
        return std::filesystem::path { __FILE__ }.parent_path().parent_path();
    }

    /** Lex, parse, analyse, and lower the fixture; null if any stage fails. */
    [[nodiscard]] auto lower(lbc::Context& context) -> std::unique_ptr<llvm::Module>;

    bool m_failed = false;
};
