//
// Created by Albert Varaksin on 16/06/2026.
//
#include "pch.hpp"
#include <gtest/gtest.h>
#include "Driver/Context.hpp"
#include "Gen/Generator.hpp"
#include "IR/gen/IrGenerator.hpp"
#include "IR/lib/Module.hpp"
#include "Parser/Parser.hpp"
#include "Sema/SemanticAnalyser.hpp"
#include <llvm/IR/Module.h>
using namespace lbc;

namespace {

/**
 * Run the full pipeline (parse → sema → IR gen → LLVM lowering) and return the
 * emitted LLVM IR as text, or an empty string if any stage fails.
 */
auto emitLlvm(const llvm::StringRef source) -> std::string {
    Context context;
    auto buffer = llvm::MemoryBuffer::getMemBufferCopy(source, "test");
    const auto id = context.getSourceMgr().AddNewSourceBuffer(std::move(buffer), llvm::SMLoc {});

    Parser parser { context, id };
    const auto parsed = parser.parse();
    if (!parsed.has_value()) {
        return {};
    }

    if (SemanticAnalyser sema { context }; !sema.analyse(**parsed)) {
        return {};
    }

    ir::gen::IrGenerator irGen { context };
    const auto ir = irGen.generate(**parsed);
    if (!ir.has_value()) {
        return {};
    }

    gen::Generator generator { context };
    const auto module = generator.generate(**ir);

    std::string out;
    llvm::raw_string_ostream os { out };
    module->print(os, nullptr);
    return out;
}

auto contains(const std::string& haystack, const llvm::StringRef needle) -> bool {
    return haystack.find(needle) != std::string::npos;
}

auto count(const std::string& haystack, const llvm::StringRef needle) -> std::size_t {
    std::size_t n = 0;
    for (auto pos = haystack.find(needle); pos != std::string::npos; pos = haystack.find(needle, pos + 1)) {
        ++n;
    }
    return n;
}

// -------------------------------------------------------------------------
// Tests
// -------------------------------------------------------------------------

TEST(GenTests, GlobalVariableWithInitializer) {
    const auto ir = emitLlvm("DIM x AS INTEGER = 42\n");
    // Exactly one global is materialised (a single declaration must not be
    // lowered twice), as a definition...
    EXPECT_EQ(count(ir, "= internal global"), 1U) << ir;
    EXPECT_TRUE(contains(ir, "@X = internal global i64 0")) << ir;
    // ...and its initialiser runs as a single store into that global in `main`.
    EXPECT_TRUE(contains(ir, "store i64 42, ptr @X")) << ir;
    EXPECT_EQ(count(ir, "store i64 42"), 1U) << ir;
}

TEST(GenTests, GlobalVariableWithoutInitializer) {
    const auto ir = emitLlvm("DIM y AS INTEGER\n");
    // Still exactly one global definition, but with no store (no initialiser).
    EXPECT_EQ(count(ir, "= internal global"), 1U) << ir;
    EXPECT_TRUE(contains(ir, "@Y = internal global i64 0")) << ir;
    EXPECT_FALSE(contains(ir, "store")) << ir;
}

} // namespace
