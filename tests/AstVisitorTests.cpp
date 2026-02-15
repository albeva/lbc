//
// Created by Albert Varaksin on 15/02/2026.
//
#include <gtest/gtest.h>
#include "Ast/AstExprPrinter.hpp"

using namespace lbc;

// Validate visitor dispatches through multiple expression node types
TEST(AstVisitorTests, ExprPrinterVisitsMultipleNodes) {
    // Build: foo(x + 42)
    AstVariableExpr callee({}, "foo");
    AstVariableExpr varX({}, "x");
    AstLiteralExpr lit42({}, LiteralValue(std::uint64_t { 42 }));
    AstBinaryExpr binExpr({}, &varX, &lit42, TokenKind::Plus);
    AstExpr* args[] = { &binExpr };
    AstCallExpr callExpr({}, &callee, std::span(args));

    AstExprPrinter printer;
    EXPECT_EQ(printer.print(callExpr), "foo(x + 42)");
}

// Validate visitor dispatches through subgroup (ExprSubGroup -> AstExrSubLeaf)
TEST(AstVisitorTests, SubGroupExprDispatch) {
    AstExrSubLeaf subLeaf({});

    AstExprPrinter printer;
    EXPECT_EQ(printer.print(subLeaf), "AstExrSubLeaf");
}

// Validate unhandled nodes fall through to the generic catch-all accept
TEST(AstVisitorTests, UnhandledFallsToGenericAccept) {
    AstVariableExpr varX({}, "x");
    AstCastExpr castExpr({}, &varX, nullptr, false);

    AstExprPrinter printer;
    EXPECT_EQ(printer.print(castExpr), "unhandled AstCastExpr");
}
