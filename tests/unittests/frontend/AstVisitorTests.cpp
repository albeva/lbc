//
// Created by Albert Varaksin on 15/02/2026.
//
#include <gtest/gtest.h>
#include "Ast/AstCodePrinter.hpp"

using namespace lbc;

// Validate visitor dispatches through multiple expression node types
TEST(AstVisitorTests, ExprPrinterVisitsMultipleNodes) {
    // Build: foo(x + 42)
    AstVarExpr callee({}, "foo");
    AstVarExpr varX({}, "x");
    AstLiteralExpr lit42({}, LiteralValue::from(std::uint64_t { 42 }));
    AstBinaryExpr binExpr({}, &varX, &lit42, TokenKind::Plus);
    AstExpr* args[] = { &binExpr };
    AstCallExpr callExpr({}, &callee, std::span(args));

    std::string output = "";
    llvm::raw_string_ostream ss { output };
    AstCodePrinter printer { ss };
    printer.print(callExpr);
    EXPECT_EQ(output, "foo((x + 42))");
}
