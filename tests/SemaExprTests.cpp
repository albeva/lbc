//
// Created by Albert Varaksin on 23/02/2026.
//
#include "pch.hpp"
#include <gtest/gtest.h>
#include "Ast/Ast.hpp"
#include "Driver/Context.hpp"
#include "Parser/Parser.hpp"
#include "Sema/SemanticAnalyser.hpp"
#include "Symbol/Symbol.hpp"
using namespace lbc;

namespace {

/**
 * Parse and analyse the source, returning the module AST.
 * Returns nullptr on failure (test will have recorded a GTest failure).
 */
auto analyse(Context& context, const llvm::StringRef source) -> AstModule* {
    auto buffer = llvm::MemoryBuffer::getMemBufferCopy(source, "test");
    const auto id = context.getSourceMgr().AddNewSourceBuffer(std::move(buffer), llvm::SMLoc {});
    Parser parser { context, id };

    const auto parsed = parser.parse();
    EXPECT_TRUE(parsed.has_value()) << "parse failed";

    auto* module = parsed.value_or(nullptr);
    EXPECT_NE(module, nullptr) << "missing ast";

    SemanticAnalyser sema { context };
    const auto result = sema.analyse(*module);
    EXPECT_TRUE(result.has_value()) << "sema failed";

    return module;
}

/**
 * Parse "DIM x = <expr>", analyse, and return the type of x.
 */
auto deduceExpr(const llvm::StringRef expr) -> const Type* {
    Context context;
    const auto source = ("DIM x = " + expr).str();
    auto* module = analyse(context, source);
    EXPECT_NE(module, nullptr);

    const auto stmts = module->getStmtList()->getStmts();
    EXPECT_EQ(stmts.size(), 1);
    auto* dim = llvm::dyn_cast<AstDimStmt>(stmts[0]);
    EXPECT_NE(dim, nullptr);

    const auto decls = dim->getDecls();
    EXPECT_EQ(decls.size(), 1);

    const Type* type = decls.front()->getType();
    EXPECT_NE(type, nullptr);
    return type;
}

/**
 * Parse "DIM x AS <typeName> = <expr>", analyse, and return the type of x.
 */
auto deduceTypedExpr(const llvm::StringRef typeName, const llvm::StringRef expr) -> const Type* {
    Context context;
    const auto source = ("DIM x AS " + typeName + " = " + expr).str();
    auto* module = analyse(context, source);
    EXPECT_NE(module, nullptr);

    const auto stmts = module->getStmtList()->getStmts();
    EXPECT_EQ(stmts.size(), 1);
    auto* dim = llvm::dyn_cast<AstDimStmt>(stmts[0]);
    EXPECT_NE(dim, nullptr);

    const auto decls = dim->getDecls();
    EXPECT_EQ(decls.size(), 1);

    const Type* type = decls.front()->getType();
    EXPECT_NE(type, nullptr);
    return type;
}

/**
 * Parse and analyse the source, expecting semantic analysis to fail.
 */
auto semaFails(const llvm::StringRef source) -> bool {
    Context context;
    auto buffer = llvm::MemoryBuffer::getMemBufferCopy(source, "test");
    const auto id = context.getSourceMgr().AddNewSourceBuffer(std::move(buffer), llvm::SMLoc {});
    Parser parser { context, id };
    const auto parsed = parser.parse();
    EXPECT_TRUE(parsed.has_value()) << "parse failed";
    auto* module = parsed.value_or(nullptr);
    EXPECT_NE(module, nullptr);
    if (module == nullptr) {
        return false;
    }
    SemanticAnalyser sema { context };
    return !sema.analyse(*module).has_value();
}

/**
 * Parse the source, expecting parsing itself to fail.
 */
auto parseFails(const llvm::StringRef source) -> bool {
    Context context;
    auto buffer = llvm::MemoryBuffer::getMemBufferCopy(source, "test");
    const auto id = context.getSourceMgr().AddNewSourceBuffer(std::move(buffer), llvm::SMLoc {});
    Parser parser { context, id };
    return !parser.parse().has_value();
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
    const auto* module = analyse(context, "DIM ip AS INTEGER PTR\nDIM b = ip = null");
    const auto stmts = module->getStmtList()->getStmts();
    ASSERT_EQ(stmts.size(), 2);
    auto* dim = llvm::dyn_cast<AstDimStmt>(stmts[1]);
    ASSERT_NE(dim, nullptr);
    EXPECT_TRUE(dim->getDecls().front()->getType()->isBool());
}

TEST(SemaExprTests, NullNotEqualPointer) {
    Context context;
    const auto* module = analyse(context, "DIM ip AS INTEGER PTR\nDIM b = ip <> null");
    const auto stmts = module->getStmtList()->getStmts();
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

// =============================================================================
// Type qualifier permutations (const / ptr / ref)
// =============================================================================

TEST(SemaExprTests, ConstPointerBaseResolves) {
    // CONST INTEGER PTR = pointer to const integer
    const auto* type = deduceTypedExpr("CONST INTEGER PTR", "null");
    ASSERT_TRUE(type->isPointer());
    const auto* pointee = type->getBaseType();
    EXPECT_TRUE(pointee->isConst());
    EXPECT_TRUE(pointee->getBaseType()->isInteger());
}

TEST(SemaExprTests, EastConstEqualsWestConst) {
    // INTEGER CONST PTR == CONST INTEGER PTR
    const auto* type = deduceTypedExpr("INTEGER CONST PTR", "null");
    ASSERT_TRUE(type->isPointer());
    EXPECT_TRUE(type->getBaseType()->isConst());
}

TEST(SemaExprTests, RedundantConstCollapses) {
    // CONST INTEGER CONST PTR collapses to a single const on the pointee
    const auto* type = deduceTypedExpr("CONST INTEGER CONST PTR", "null");
    ASSERT_TRUE(type->isPointer());
    const auto* pointee = type->getBaseType();
    ASSERT_TRUE(pointee->isConst());
    EXPECT_TRUE(pointee->getBaseType()->isInteger()); // not const-of-const
}

TEST(SemaExprTests, PointerToPointerResolves) {
    const auto* type = deduceTypedExpr("INTEGER PTR PTR", "null");
    ASSERT_TRUE(type->isPointer());
    EXPECT_TRUE(type->getBaseType()->isPointer());
}

TEST(SemaExprTests, ReferenceToReferenceRejected) {
    // REF must be the last qualifier — rejected by the parser.
    EXPECT_TRUE(parseFails("DIM x AS INTEGER REF REF"));
}

TEST(SemaExprTests, PointerToReferenceRejected) {
    EXPECT_TRUE(parseFails("DIM x AS INTEGER REF PTR"));
}

TEST(SemaExprTests, ConstToReferenceRejected) {
    EXPECT_TRUE(parseFails("DIM x AS INTEGER REF CONST"));
}

TEST(SemaExprTests, DoubleConstPrefixRejected) {
    EXPECT_TRUE(parseFails("DIM x AS CONST CONST INTEGER"));
}

// =============================================================================
// Value categories
// =============================================================================

namespace {

/** Analyse @p source and return the initialiser expression of the DIM at @p index. */
auto dimInitExpr(Context& context, const llvm::StringRef source, const std::size_t index) -> AstExpr* {
    auto* module = analyse(context, source);
    EXPECT_NE(module, nullptr);
    if (module == nullptr) {
        return nullptr;
    }
    const auto stmts = module->getStmtList()->getStmts();
    EXPECT_GT(stmts.size(), index);
    if (stmts.size() <= index) {
        return nullptr;
    }
    auto* dim = llvm::dyn_cast<AstDimStmt>(stmts[index]);
    EXPECT_NE(dim, nullptr);
    if (dim == nullptr) {
        return nullptr;
    }
    return dim->getDecls().front()->getExpr();
}

} // namespace

TEST(SemaExprTests, LiteralIsValue) {
    Context context;
    auto* expr = dimInitExpr(context, "DIM a = 42", 0);
    ASSERT_NE(expr, nullptr);
    EXPECT_TRUE(expr->getValueCategory().isValue());
}

TEST(SemaExprTests, VariableIsAddressable) {
    Context context;
    auto* expr = dimInitExpr(context, "DIM a = 42\nDIM b = a", 1);
    ASSERT_NE(expr, nullptr);
    EXPECT_TRUE(expr->getValueCategory().isAddressable());
}

TEST(SemaExprTests, DereferenceIsAddressable) {
    Context context;
    auto* expr = dimInitExpr(context, "DIM p AS INTEGER PTR\nDIM v = *p", 1);
    ASSERT_NE(expr, nullptr);
    EXPECT_TRUE(expr->getValueCategory().isAddressable());
}

TEST(SemaExprTests, AddressOfIsValue) {
    Context context;
    auto* expr = dimInitExpr(context, "DIM a = 42\nDIM p = @a", 1);
    ASSERT_NE(expr, nullptr);
    EXPECT_TRUE(expr->getValueCategory().isValue());
}

TEST(SemaExprTests, BinaryIsValue) {
    Context context;
    auto* expr = dimInitExpr(context, "DIM a = 1 + 2", 0);
    ASSERT_NE(expr, nullptr);
    EXPECT_TRUE(expr->getValueCategory().isValue());
}

// =============================================================================
// Assignability (requires a non-const Place on the left-hand side)
// =============================================================================

TEST(SemaExprTests, AssignToVariableAccepted) {
    EXPECT_FALSE(semaFails("DIM a = 1\na = 2"));
}

TEST(SemaExprTests, AssignThroughPointerAccepted) {
    // *p designates an object (Addressable), so it is assignable.
    EXPECT_FALSE(semaFails("DIM p AS INTEGER PTR\n*p = 42"));
}

TEST(SemaExprTests, AssignToLiteralRejected) {
    EXPECT_TRUE(semaFails("42 = 1"));
}

TEST(SemaExprTests, AssignToBinaryRejected) {
    EXPECT_TRUE(semaFails("DIM a = 1\na + 1 = 2"));
}

TEST(SemaExprTests, AssignThroughConstPointerRejected) {
    // *p has type CONST INTEGER: a Place, but const, so not assignable.
    EXPECT_TRUE(semaFails("DIM p AS CONST INTEGER PTR = null\n*p = 1"));
}

// =============================================================================
// Address-of (requires a Place operand)
// =============================================================================

TEST(SemaExprTests, AddressOfDereferenceAccepted) {
    // @(*p) — dereference is addressable, so its address can be taken.
    EXPECT_FALSE(semaFails("DIM p AS INTEGER PTR\nDIM q = @(*p)"));
}

TEST(SemaExprTests, AddressOfBinaryRejected) {
    EXPECT_TRUE(semaFails("DIM a = 1\nDIM p = @(a + 1)"));
}

// =============================================================================
// MOVE — produces an xvalue (Expiring)
// =============================================================================

TEST(SemaExprTests, MoveProducesExpiring) {
    Context context;
    auto* expr = dimInitExpr(context, "DIM a = 42\nDIM b = MOVE a", 1);
    ASSERT_NE(expr, nullptr);
    EXPECT_TRUE(expr->getValueCategory().isExpiring());
}

TEST(SemaExprTests, MoveIsMovableGlvalue) {
    Context context;
    auto* expr = dimInitExpr(context, "DIM a = 42\nDIM b = MOVE a", 1);
    ASSERT_NE(expr, nullptr);
    const auto cat = expr->getValueCategory();
    EXPECT_TRUE(cat.isMovable());   // rvalue
    EXPECT_TRUE(cat.hasIdentity()); // glvalue
}

TEST(SemaExprTests, MovePreservesType) {
    Context context;
    auto* expr = dimInitExpr(context, "DIM a = 42\nDIM b = MOVE a", 1);
    ASSERT_NE(expr, nullptr);
    EXPECT_TRUE(expr->getType()->isInteger());
}

TEST(SemaExprTests, MoveOfTemporaryRejected) {
    EXPECT_TRUE(semaFails("DIM x = MOVE 42"));
}

TEST(SemaExprTests, MoveOfBinaryRejected) {
    EXPECT_TRUE(semaFails("DIM a = 1\nDIM x = MOVE (a + 1)"));
}

TEST(SemaExprTests, MoveResultNotAddressable) {
    // The result of MOVE is an xvalue, not addressable — its address cannot be taken.
    EXPECT_TRUE(semaFails("DIM a = 42\nDIM p = @(MOVE a)"));
}

TEST(SemaExprTests, AssignToMoveResultRejected) {
    // The result of MOVE is an xvalue (Expiring), not addressable, so it cannot
    // be the target of an assignment.
    EXPECT_TRUE(semaFails("DIM a = 1\n(MOVE a) = 5"));
}

// =============================================================================
// Call expressions — category derives from the return type
// =============================================================================

TEST(SemaExprTests, CallReturningValueIsValue) {
    Context context;
    auto* expr = dimInitExpr(context, "DECLARE FUNCTION f() AS INTEGER\nDIM x = f()", 1);
    ASSERT_NE(expr, nullptr);
    EXPECT_TRUE(llvm::isa<AstCallExpr>(expr));
    EXPECT_TRUE(expr->getValueCategory().isValue());
}

TEST(SemaExprTests, CallReturningReferenceIsAddressable) {
    Context context;
    auto* expr = dimInitExpr(context, "DECLARE FUNCTION f() AS INTEGER REF\nDIM x = f()", 1);
    ASSERT_NE(expr, nullptr);
    EXPECT_TRUE(llvm::isa<AstCallExpr>(expr));
    EXPECT_TRUE(expr->getValueCategory().isAddressable());
}

TEST(SemaExprTests, CallReturningReferenceStripsToReferentType) {
    // The expression type is the referent (INTEGER), never a reference type;
    // the reference manifests as the Addressable category instead.
    Context context;
    auto* expr = dimInitExpr(context, "DECLARE FUNCTION f() AS INTEGER REF\nDIM x = f()", 1);
    ASSERT_NE(expr, nullptr);
    EXPECT_TRUE(expr->getType()->isInteger());
    EXPECT_FALSE(expr->getType()->isReference());
}

// =============================================================================
// Reference binding (DIM ... REF) requires an addressable initialiser
// =============================================================================

TEST(SemaExprTests, ReferenceBindsToAddressable) {
    EXPECT_FALSE(semaFails("DIM a = 1\nDIM b AS INTEGER REF = a"));
}

TEST(SemaExprTests, ReferenceToBinaryRejected) {
    EXPECT_TRUE(semaFails("DIM a = 1\nDIM b AS INTEGER REF = a + 1"));
}

// =============================================================================
// Implicit conversion (the lvalue-to-rvalue load) yields a Value
// =============================================================================

TEST(SemaExprTests, WideningConversionIsValue) {
    // Reading an Addressable variable into a wider type inserts an implicit
    // cast; the converted result is a pure Value (prvalue), not a place.
    Context context;
    auto* expr = dimInitExpr(context, "DIM v = 1\nDIM x AS LONG = v", 1);
    ASSERT_NE(expr, nullptr);
    EXPECT_TRUE(llvm::isa<AstCastExpr>(expr));
    EXPECT_TRUE(expr->getValueCategory().isValue());
    EXPECT_TRUE(expr->getType()->isLong());
}

TEST(SemaExprTests, ComparisonIsValue) {
    Context context;
    auto* expr = dimInitExpr(context, "DIM a = 1\nDIM b = a < 2", 1);
    ASSERT_NE(expr, nullptr);
    EXPECT_TRUE(expr->getValueCategory().isValue());
}

// =============================================================================
// EXTERN "C" linkage block (AstExtern) — verbatim symbol alias
// =============================================================================

namespace {

/** Analyse @p source and return the module's first statement. */
auto firstStmt(Context& context, const llvm::StringRef source) -> AstStmt* {
    auto* module = analyse(context, source);
    EXPECT_NE(module, nullptr);
    if (module == nullptr) {
        return nullptr;
    }
    const auto stmts = module->getStmtList()->getStmts();
    EXPECT_FALSE(stmts.empty());
    return stmts.empty() ? nullptr : stmts[0];
}

} // namespace

namespace {
/** The Symbol of the i-th DECLARE inside an extern block. */
auto externDeclSymbol(const AstExtern* ext, const std::size_t index) -> const Symbol* {
    return llvm::cast<AstDeclareStmt>(ext->getStmts()[index])->getDecl()->getSymbol();
}
} // namespace

TEST(SemaExprTests, ExternCSingleLineCreatesBlock) {
    Context context;
    auto* ext = llvm::dyn_cast_or_null<AstExtern>(
        firstStmt(context, "EXTERN \"C\" DECLARE SUB puts(s AS ZSTRING)\nputs \"hi\"")
    );
    ASSERT_NE(ext, nullptr);
    EXPECT_TRUE(ext->getExternKind() == ExternKind::C); // resolved linkage on the node
    EXPECT_EQ(ext->getStmts().size(), 1u);
}

TEST(SemaExprTests, ExternCSetsVerbatimAlias) {
    Context context;
    auto* ext = llvm::dyn_cast_or_null<AstExtern>(
        firstStmt(context, "EXTERN \"C\" DECLARE SUB puts(s AS ZSTRING)\nputs \"hi\"")
    );
    ASSERT_NE(ext, nullptr);
    ASSERT_EQ(ext->getStmts().size(), 1u);
    const auto* symbol = externDeclSymbol(ext, 0);
    ASSERT_NE(symbol, nullptr);
    EXPECT_TRUE(symbol->getExternKind() == ExternKind::C); // linkage is a symbol attribute
    EXPECT_EQ(symbol->getAlias(), "puts");                 // verbatim, case-preserved
    EXPECT_EQ(symbol->getSymbolName(), "puts");            // emitted name prefers the alias
    EXPECT_EQ(symbol->getName(), "PUTS");                  // canonical name is still upper-cased
}

TEST(SemaExprTests, ExternCBlockFormAliasesAll) {
    Context context;
    auto* ext = llvm::dyn_cast_or_null<AstExtern>(firstStmt(context, "EXTERN \"C\"\n"
                                                                     "DECLARE SUB puts(s AS ZSTRING)\n"
                                                                     "DECLARE FUNCTION getchar() AS INTEGER\n"
                                                                     "END EXTERN\n"));
    ASSERT_NE(ext, nullptr);
    ASSERT_EQ(ext->getStmts().size(), 2u);
    EXPECT_EQ(externDeclSymbol(ext, 0)->getAlias(), "puts");
    EXPECT_EQ(externDeclSymbol(ext, 1)->getAlias(), "getchar");
}

TEST(SemaExprTests, ExternCLowercaseLanguageAccepted) {
    // The language string is matched case-insensitively, so "c" works too.
    EXPECT_FALSE(semaFails("EXTERN \"c\" DECLARE SUB puts(s AS ZSTRING)\nputs \"hi\""));
}

TEST(SemaExprTests, NonExternDeclareHasNoAlias) {
    Context context;
    auto* decl = llvm::dyn_cast_or_null<AstDeclareStmt>(firstStmt(context, "DECLARE SUB foo(x AS INTEGER)"));
    ASSERT_NE(decl, nullptr);
    const auto* symbol = decl->getDecl()->getSymbol();
    ASSERT_NE(symbol, nullptr);
    EXPECT_TRUE(symbol->getAlias().empty());
    EXPECT_TRUE(symbol->getExternKind() == ExternKind::Default);
    EXPECT_EQ(symbol->getSymbolName(), "FOO"); // no alias → canonical upper-cased name
}

TEST(SemaExprTests, ExternUnsupportedLanguageRejected) {
    // Unsupported linkage is rejected by the parser.
    EXPECT_TRUE(parseFails("EXTERN \"Rust\" DECLARE SUB foo(x AS INTEGER)"));
}
