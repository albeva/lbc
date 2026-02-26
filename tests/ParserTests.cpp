//
// Created by Albert Varaksin on 18/02/2026.
//
#include "pch.hpp"
#include <gtest/gtest.h>
#include "Ast/Ast.hpp"
#include "Ast/AstCodePrinter.hpp"
#include "Driver/Context.hpp"
#include "Parser/Parser.hpp"
using namespace lbc;

namespace {

/**
 * Parse "DIM x = <expr>" and return the printed expression string.
 */
auto parseExpr(llvm::StringRef expr) -> std::string {
    Context context;
    auto source = ("DIM x = " + expr).str();
    auto buffer = llvm::MemoryBuffer::getMemBufferCopy(source, "test");
    auto id = context.getSourceMgr().AddNewSourceBuffer(std::move(buffer), llvm::SMLoc {});
    Parser parser { context, id };

    auto result = parser.parse();
    EXPECT_TRUE(result.has_value()) << "parse failed";

    auto* module = result.value_or(nullptr);
    EXPECT_NE(module, nullptr) << "missing module";

    auto stmts = module->getStmtList()->getStmts();
    EXPECT_EQ(stmts.size(), 1);

    auto* dim = llvm::dyn_cast<AstDimStmt>(stmts[0]);
    EXPECT_NE(dim, nullptr);

    auto decls = dim->getDecls();
    EXPECT_EQ(decls.size(), 1);

    auto* varExpr = decls[0]->getExpr();
    EXPECT_NE(varExpr, nullptr);

    std::string output = "";
    llvm::raw_string_ostream ss { output };
    AstCodePrinter printer { ss };
    printer.print(*varExpr);
    return output;
}

} // namespace

// ------------------------------------
// Literals
// ------------------------------------

TEST(ParserTests, IntegerLiteral) {
    EXPECT_EQ(parseExpr("42"), "42");
}

TEST(ParserTests, FloatLiteral) {
    EXPECT_EQ(parseExpr("3.14"), "3.140000");
}

TEST(ParserTests, BooleanLiteral) {
    EXPECT_EQ(parseExpr("true"), "true");
    EXPECT_EQ(parseExpr("false"), "false");
}

TEST(ParserTests, StringLiteral) {
    EXPECT_EQ(parseExpr("\"hello\""), "\"hello\"");
}

TEST(ParserTests, NullLiteral) {
    EXPECT_EQ(parseExpr("null"), "null");
}

// ------------------------------------
// Variables
// ------------------------------------

TEST(ParserTests, Variable) {
    EXPECT_EQ(parseExpr("foo"), "FOO");
}

// ------------------------------------
// Binary expressions
// ------------------------------------

TEST(ParserTests, BinaryAdd) {
    EXPECT_EQ(parseExpr("1 + 2"), "(1 + 2)");
}

TEST(ParserTests, BinaryPrecedence) {
    EXPECT_EQ(parseExpr("1 + 2 * 3"), "(1 + (2 * 3))");
}

TEST(ParserTests, BinaryLeftAssociativity) {
    EXPECT_EQ(parseExpr("1 - 2 - 3"), "((1 - 2) - 3)");
}

TEST(ParserTests, BinaryMultipleOperators) {
    EXPECT_EQ(parseExpr("a + b * c - d"), "((A + (B * C)) - D)");
}

// ------------------------------------
// Unary expressions
// ------------------------------------

TEST(ParserTests, UnaryNegate) {
    EXPECT_EQ(parseExpr("-x"), "(-X)");
}

TEST(ParserTests, UnaryWithBinary) {
    EXPECT_EQ(parseExpr("-x + y"), "((-X) + Y)");
}

// ------------------------------------
// Parenthesised expressions
// ------------------------------------

TEST(ParserTests, Parenthesised) {
    EXPECT_EQ(parseExpr("(1 + 2) * 3"), "((1 + 2) * 3)");
}

TEST(ParserTests, NestedParentheses) {
    EXPECT_EQ(parseExpr("((a))"), "A");
}

// ------------------------------------
// Function calls
// ------------------------------------

TEST(ParserTests, FunctionCallNoArgs) {
    EXPECT_EQ(parseExpr("foo()"), "FOO()");
}

TEST(ParserTests, FunctionCallOneArg) {
    EXPECT_EQ(parseExpr("foo(1)"), "FOO(1)");
}

TEST(ParserTests, FunctionCallMultipleArgs) {
    EXPECT_EQ(parseExpr("foo(1, 2, 3)"), "FOO(1, 2, 3)");
}

TEST(ParserTests, FunctionCallExprArg) {
    EXPECT_EQ(parseExpr("foo(a + b)"), "FOO((A + B))");
}
