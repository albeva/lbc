//
// Created by Albert Varaksin on 23/02/2026.
//
#include "pch.hpp"
#include <gtest/gtest.h>
#include "Ast/Ast.hpp"
#include "Driver/Context.hpp"
#include "Parser/Parser.hpp"
#include "Sema/SemanticAnalyser.hpp"
using namespace lbc;

namespace {

/**
 * Parse and analyse the source, returning the module AST.
 * Returns nullptr on failure (test will have recorded a GTest failure).
 */
auto analyse(Context& context, llvm::StringRef source) -> AstModule* {
    auto buffer = llvm::MemoryBuffer::getMemBufferCopy(source, "test");
    auto id = context.getSourceMgr().AddNewSourceBuffer(std::move(buffer), llvm::SMLoc {});
    Parser parser { context, id };

    auto parsed = parser.parse();
    EXPECT_TRUE(parsed.has_value()) << "parse failed";

    auto* module = parsed.value_or(nullptr);
    EXPECT_NE(module, nullptr) << "missing ast";

    SemanticAnalyser sema { context };
    auto result = sema.analyse(*module);
    EXPECT_TRUE(result.has_value()) << "sema failed";

    return module;
}

/**
 * Parse "DIM x = <expr>", analyse, and return the type of x.
 */
auto deduceExpr(llvm::StringRef expr) -> const Type* {
    Context context;
    auto source = ("DIM x = " + expr).str();
    auto* module = analyse(context, source);
    EXPECT_NE(module, nullptr);

    auto stmts = module->getStmtList()->getStmts();
    EXPECT_EQ(stmts.size(), 1);
    auto* dim = llvm::dyn_cast<AstDimStmt>(stmts[0]);
    EXPECT_NE(dim, nullptr);

    auto decls = dim->getDecls();
    EXPECT_EQ(decls.size(), 1);

    const Type* type = decls.front()->getType();
    EXPECT_NE(type, nullptr);
    return type;
}

/**
 * Parse "DIM x AS <typeName> = <expr>", analyse, and return the type of x.
 */
auto deduceTypedExpr(llvm::StringRef typeName, llvm::StringRef expr) -> const Type* {
    Context context;
    auto source = ("DIM x AS " + typeName + " = " + expr).str();
    auto* module = analyse(context, source);
    EXPECT_NE(module, nullptr);

    auto stmts = module->getStmtList()->getStmts();
    EXPECT_EQ(stmts.size(), 1);
    auto* dim = llvm::dyn_cast<AstDimStmt>(stmts[0]);
    EXPECT_NE(dim, nullptr);

    auto decls = dim->getDecls();
    EXPECT_EQ(decls.size(), 1);

    const Type* type = decls.front()->getType();
    EXPECT_NE(type, nullptr);
    return type;
}

/**
 * Parse and analyse the source, expecting semantic analysis to fail.
 */
auto semaFails(llvm::StringRef source) -> bool {
    Context context;
    auto buffer = llvm::MemoryBuffer::getMemBufferCopy(source, "test");
    auto id = context.getSourceMgr().AddNewSourceBuffer(std::move(buffer), llvm::SMLoc {});
    Parser parser { context, id };
    auto parsed = parser.parse();
    EXPECT_TRUE(parsed.has_value()) << "parse failed";
    auto* module = parsed.value_or(nullptr);
    EXPECT_NE(module, nullptr);
    if (module == nullptr) { return false; }
    SemanticAnalyser sema { context };
    return !sema.analyse(*module).has_value();
}

} // namespace

// =============================================================================
// Literal type deduction
// =============================================================================

TEST(SemaExprTests, IntegerLiteralDeducesInteger) {
    const auto* type = deduceExpr("42");
    EXPECT_TRUE(type->isInteger());
}

TEST(SemaExprTests, FloatLiteralDeducesDouble) {
    const auto* type = deduceExpr("3.14");
    EXPECT_TRUE(type->isDouble());
}

TEST(SemaExprTests, BoolLiteralDeducesBool) {
    const auto* type = deduceExpr("true");
    EXPECT_TRUE(type->isBool());
}

TEST(SemaExprTests, StringLiteralDeducesZString) {
    const auto* type = deduceExpr("\"hello\"");
    EXPECT_TRUE(type->isZString());
}

// =============================================================================
// Explicit type on DIM coerces literal
// =============================================================================

TEST(SemaExprTests, IntegerLiteralCoercesToByte) {
    const auto* type = deduceTypedExpr("Byte", "42");
    EXPECT_TRUE(type->isByte());
}

TEST(SemaExprTests, IntegerLiteralCoercesToLong) {
    const auto* type = deduceTypedExpr("Long", "42");
    EXPECT_TRUE(type->isLong());
}

TEST(SemaExprTests, FloatLiteralCoercesToSingle) {
    const auto* type = deduceTypedExpr("Single", "3.14");
    EXPECT_TRUE(type->isSingle());
}

// =============================================================================
// Binary expression type deduction
// =============================================================================

TEST(SemaExprTests, IntegerAddition) {
    const auto* type = deduceExpr("1 + 2");
    EXPECT_TRUE(type->isInteger());
}

TEST(SemaExprTests, FloatAddition) {
    const auto* type = deduceExpr("1.0 + 2.0");
    EXPECT_TRUE(type->isDouble());
}

TEST(SemaExprTests, ComparisonDeducesBool) {
    const auto* type = deduceExpr("1 < 2");
    EXPECT_TRUE(type->isBool());
}

TEST(SemaExprTests, LogicalAndDeducesBool) {
    const auto* type = deduceExpr("true AND false");
    EXPECT_TRUE(type->isBool());
}

// =============================================================================
// Unary expression type deduction
// =============================================================================

TEST(SemaExprTests, NegateInteger) {
    const auto* type = deduceExpr("-42");
    EXPECT_TRUE(type->isInteger());
}

TEST(SemaExprTests, NegateFloat) {
    const auto* type = deduceExpr("-3.14");
    EXPECT_TRUE(type->isDouble());
}

TEST(SemaExprTests, LogicalNotBool) {
    const auto* type = deduceExpr("NOT true");
    EXPECT_TRUE(type->isBool());
}

// =============================================================================
// Null semantics
// =============================================================================

TEST(SemaExprTests, NullAssignedToPointer) {
    const auto* type = deduceTypedExpr("INTEGER PTR", "null");
    EXPECT_TRUE(type->isPointer());
}

TEST(SemaExprTests, NullEqualNullDeducesBool) {
    const auto* type = deduceExpr("null = null");
    EXPECT_TRUE(type->isBool());
}

TEST(SemaExprTests, NullNotEqualNullDeducesBool) {
    const auto* type = deduceExpr("null <> null");
    EXPECT_TRUE(type->isBool());
}

TEST(SemaExprTests, NullComparedWithPointer) {
    Context context;
    auto* module = analyse(context, "DIM ip AS INTEGER PTR\nDIM b = ip = null");
    auto stmts = module->getStmtList()->getStmts();
    ASSERT_EQ(stmts.size(), 2);
    auto* dim = llvm::dyn_cast<AstDimStmt>(stmts[1]);
    ASSERT_NE(dim, nullptr);
    EXPECT_TRUE(dim->getDecls().front()->getType()->isBool());
}

TEST(SemaExprTests, NullNotEqualPointer) {
    Context context;
    auto* module = analyse(context, "DIM ip AS INTEGER PTR\nDIM b = ip <> null");
    auto stmts = module->getStmtList()->getStmts();
    ASSERT_EQ(stmts.size(), 2);
    auto* dim = llvm::dyn_cast<AstDimStmt>(stmts[1]);
    ASSERT_NE(dim, nullptr);
    EXPECT_TRUE(dim->getDecls().front()->getType()->isBool());
}

TEST(SemaExprTests, NullVariableRejected) {
    EXPECT_TRUE(semaFails("DIM x = null"));
}

TEST(SemaExprTests, AddressOfNullRejected) {
    EXPECT_TRUE(semaFails("DIM x AS INTEGER PTR = @null"));
}

TEST(SemaExprTests, NullToReferenceRejected) {
    EXPECT_TRUE(semaFails("DIM x AS INTEGER REF = null"));
}

// =============================================================================
// Error paths — unary
// =============================================================================

TEST(SemaExprTests, NegateNonNumericRejected) {
    EXPECT_TRUE(semaFails("DIM x = -true"));
}

TEST(SemaExprTests, LogicalNotNonBoolRejected) {
    EXPECT_TRUE(semaFails("DIM x = NOT 42"));
}

TEST(SemaExprTests, DereferenceNonPointerRejected) {
    EXPECT_TRUE(semaFails("DIM x = *42"));
}

// =============================================================================
// Error paths — binary
// =============================================================================

TEST(SemaExprTests, AddStringToIntegerRejected) {
    EXPECT_TRUE(semaFails("DIM x = 1 + \"hello\""));
}

TEST(SemaExprTests, LogicalAndIntegerRejected) {
    EXPECT_TRUE(semaFails("DIM x = 1 AND 2"));
}

// =============================================================================
// Explicit cast (AS) — requires parser support for suffix expressions
// =============================================================================

// TODO: enable once AS is parsed
// TEST(SemaExprTests, CastIntegerToByte)
// TEST(SemaExprTests, CastPropagatesInBinaryExpr)
