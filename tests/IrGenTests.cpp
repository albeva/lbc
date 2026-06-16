//
// Created by Albert Varaksin on 04/04/2026.
//
#include "pch.hpp"
#include <gtest/gtest.h>
#include "Ast/Ast.hpp"
#include "Driver/Context.hpp"
#include "IR/gen/IrGenerator.hpp"
#include "IR/lib/Module.hpp"
#include "Parser/Parser.hpp"
#include "Sema/SemanticAnalyser.hpp"
using namespace lbc;

namespace {

/**
 * Run the full pipeline (parse → sema → IR gen) and return
 * whether IR generation succeeded or failed.
 */
auto irGenSucceeds(const llvm::StringRef source) -> bool {
    Context context;
    auto buffer = llvm::MemoryBuffer::getMemBufferCopy(source, "test");
    const auto id = context.getSourceMgr().AddNewSourceBuffer(std::move(buffer), llvm::SMLoc {});
    Parser parser { context, id };

    const auto parsed = parser.parse();
    if (!parsed.has_value()) {
        return false;
    }

    if (SemanticAnalyser sema { context }; !sema.analyse(**parsed)) {
        return false;
    }

    ir::gen::IrGenerator gen { context };
    return gen.generate(**parsed).has_value();
}

// -------------------------------------------------------------------------
// Tests
// -------------------------------------------------------------------------

TEST(IrGenTests, DeclareStmtSucceeds) {
    EXPECT_TRUE(irGenSucceeds("DECLARE SUB foo()\n"));
}

TEST(IrGenTests, DimStmtSucceeds) {
    EXPECT_TRUE(irGenSucceeds("DIM x AS INTEGER\n"));
}

TEST(IrGenTests, FuncStmtSucceeds) {
    EXPECT_TRUE(irGenSucceeds(
        "SUB foo()\n"
        "END SUB\n"
    ));
}

TEST(IrGenTests, FuncWithParamsSucceeds) {
    EXPECT_TRUE(irGenSucceeds(
        "FUNCTION add(a AS INTEGER, b AS INTEGER) AS INTEGER\n"
        "    RETURN a + b\n"
        "END FUNCTION\n"
    ));
}

TEST(IrGenTests, ExternVariadicSucceeds) {
    EXPECT_TRUE(irGenSucceeds("EXTERN \"C\" DECLARE FUNCTION printf(fmt AS ZSTRING, ...) AS INTEGER\n"));
}

} // namespace
