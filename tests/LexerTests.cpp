#include <gtest/gtest.h>
#include "Driver/Context.hpp"
#include "Lexer/Lexer.hpp"
using namespace lbc;

namespace {

/**
 * Create a lexer from a source string.
 */
auto makeLexer(Context& context, llvm::StringRef source) -> Lexer {
    auto buffer = llvm::MemoryBuffer::getMemBufferCopy(source, "test");
    auto id = context.getSourceMgr().AddNewSourceBuffer(std::move(buffer), llvm::SMLoc {});
    return Lexer { context, id };
}

} // namespace

// ------------------------------------
// Comments
// ------------------------------------

TEST(LexerTests, SingleLineComment) {
    Context context;
    auto lexer = makeLexer(context, "' this is a comment\n42");
    auto tok = lexer.next();
    EXPECT_EQ(tok.kind(), TokenKind::IntegerLiteral);
}

TEST(LexerTests, MultilineComment) {
    Context context;
    auto lexer = makeLexer(context, "/' comment '/ 42");
    auto tok = lexer.next();
    EXPECT_EQ(tok.kind(), TokenKind::IntegerLiteral);
}

TEST(LexerTests, NestedMultilineComment) {
    Context context;
    auto lexer = makeLexer(context, "/' outer\n/'\ninner\n'/\nstill outer '/ 42");
    auto tok = lexer.next();
    EXPECT_EQ(tok.kind(), TokenKind::IntegerLiteral);
}

TEST(LexerTests, UnclosedMultilineCommentReachesEof) {
    Context context;
    auto lexer = makeLexer(context, "/' unclosed comment");
    auto tok = lexer.next();
    EXPECT_EQ(tok.kind(), TokenKind::EndOfFile);
}

// ------------------------------------
// Newlines and statements
// ------------------------------------

TEST(LexerTests, NewlineProducesEndOfStmt) {
    Context context;
    auto lexer = makeLexer(context, "42\n43");
    EXPECT_EQ(lexer.next().kind(), TokenKind::IntegerLiteral);
    EXPECT_EQ(lexer.next().kind(), TokenKind::EndOfStmt);
    EXPECT_EQ(lexer.next().kind(), TokenKind::IntegerLiteral);
}

TEST(LexerTests, LeadingNewlinesAreSkipped) {
    Context context;
    auto lexer = makeLexer(context, "\n\n\n42");
    EXPECT_EQ(lexer.next().kind(), TokenKind::IntegerLiteral);
}

TEST(LexerTests, ConsecutiveNewlinesProduceSingleEndOfStmt) {
    Context context;
    auto lexer = makeLexer(context, "42\n\n\n43");
    EXPECT_EQ(lexer.next().kind(), TokenKind::IntegerLiteral);
    EXPECT_EQ(lexer.next().kind(), TokenKind::EndOfStmt);
    EXPECT_EQ(lexer.next().kind(), TokenKind::IntegerLiteral);
}

TEST(LexerTests, CarriageReturnLineFeed) {
    Context context;
    auto lexer = makeLexer(context, "42\r\n43");
    EXPECT_EQ(lexer.next().kind(), TokenKind::IntegerLiteral);
    EXPECT_EQ(lexer.next().kind(), TokenKind::EndOfStmt);
    EXPECT_EQ(lexer.next().kind(), TokenKind::IntegerLiteral);
}

TEST(LexerTests, LineContinuation) {
    Context context;
    auto lexer = makeLexer(context, "42 _\n+ 43");
    EXPECT_EQ(lexer.next().kind(), TokenKind::IntegerLiteral);
    EXPECT_EQ(lexer.next().kind(), TokenKind::Plus);
    EXPECT_EQ(lexer.next().kind(), TokenKind::IntegerLiteral);
}

// ------------------------------------
// Literals
// ------------------------------------

TEST(LexerTests, BooleanLiteralTrue) {
    Context context;
    auto lexer = makeLexer(context, "True");
    auto tok = lexer.next();
    EXPECT_EQ(tok.kind(), TokenKind::BooleanLiteral);
    EXPECT_EQ(std::get<bool>(tok.getValue()), true);
}

TEST(LexerTests, BooleanLiteralFalse) {
    Context context;
    auto lexer = makeLexer(context, "FALSE");
    auto tok = lexer.next();
    EXPECT_EQ(tok.kind(), TokenKind::BooleanLiteral);
    EXPECT_EQ(std::get<bool>(tok.getValue()), false);
}

TEST(LexerTests, NullLiteral) {
    Context context;
    auto lexer = makeLexer(context, "Null");
    auto tok = lexer.next();
    EXPECT_EQ(tok.kind(), TokenKind::NullLiteral);
}

TEST(LexerTests, StringLiteral) {
    Context context;
    auto lexer = makeLexer(context, "\"hello world\"");
    auto tok = lexer.next();
    EXPECT_EQ(tok.kind(), TokenKind::StringLiteral);
    EXPECT_EQ(std::get<llvm::StringRef>(tok.getValue()), "hello world");
}

TEST(LexerTests, UnclosedStringLiteral) {
    Context context;
    auto lexer = makeLexer(context, "\"unclosed");
    EXPECT_EQ(lexer.next().kind(), TokenKind::Invalid);
}

TEST(LexerTests, IntegerLiteral) {
    Context context;
    auto lexer = makeLexer(context, "12345");
    auto tok = lexer.next();
    EXPECT_EQ(tok.kind(), TokenKind::IntegerLiteral);
    EXPECT_EQ(std::get<std::uint64_t>(tok.getValue()), 12345U);
}

TEST(LexerTests, FloatLiteral) {
    Context context;
    auto lexer = makeLexer(context, "3.14");
    auto tok = lexer.next();
    EXPECT_EQ(tok.kind(), TokenKind::FloatLiteral);
    EXPECT_DOUBLE_EQ(std::get<double>(tok.getValue()), 3.14);
}

TEST(LexerTests, FloatLiteralStartingWithDot) {
    Context context;
    auto lexer = makeLexer(context, ".5");
    auto tok = lexer.next();
    EXPECT_EQ(tok.kind(), TokenKind::FloatLiteral);
    EXPECT_DOUBLE_EQ(std::get<double>(tok.getValue()), 0.5);
}

TEST(LexerTests, InvalidNumberWithTrailingAlpha) {
    Context context;
    auto lexer = makeLexer(context, "123abc");
    EXPECT_EQ(lexer.next().kind(), TokenKind::Invalid);
}

// ------------------------------------
// Operators and symbols
// ------------------------------------

TEST(LexerTests, AssignOperator) {
    Context context;
    auto lexer = makeLexer(context, "=");
    EXPECT_EQ(lexer.next().kind(), TokenKind::Assign);
}

TEST(LexerTests, LessGreaterIsNotEqual) {
    Context context;
    auto lexer = makeLexer(context, "<>");
    EXPECT_EQ(lexer.next().kind(), TokenKind::NotEqual);
}

TEST(LexerTests, LessOrEqual) {
    Context context;
    auto lexer = makeLexer(context, "<=");
    EXPECT_EQ(lexer.next().kind(), TokenKind::LessOrEqual);
}

TEST(LexerTests, GreaterOrEqual) {
    Context context;
    auto lexer = makeLexer(context, ">=");
    EXPECT_EQ(lexer.next().kind(), TokenKind::GreaterOrEqual);
}

TEST(LexerTests, LessThanAlone) {
    Context context;
    auto lexer = makeLexer(context, "< 1");
    EXPECT_EQ(lexer.next().kind(), TokenKind::LessThan);
}

TEST(LexerTests, MemberAccessDot) {
    Context context;
    auto lexer = makeLexer(context, "a.b");
    EXPECT_EQ(lexer.next().kind(), TokenKind::Identifier);
    EXPECT_EQ(lexer.next().kind(), TokenKind::MemberAccess);
    EXPECT_EQ(lexer.next().kind(), TokenKind::Identifier);
}

TEST(LexerTests, Ellipsis) {
    Context context;
    auto lexer = makeLexer(context, "...");
    EXPECT_EQ(lexer.next().kind(), TokenKind::Ellipsis);
}

TEST(LexerTests, DotFollowedByDigitIsFloat) {
    Context context;
    auto lexer = makeLexer(context, ".5");
    EXPECT_EQ(lexer.next().kind(), TokenKind::FloatLiteral);
}

TEST(LexerTests, TwoDotsIsInvalid) {
    Context context;
    auto lexer = makeLexer(context, "..");
    EXPECT_EQ(lexer.next().kind(), TokenKind::Invalid);
}

TEST(LexerTests, PointerAccess) {
    Context context;
    auto lexer = makeLexer(context, "->");
    EXPECT_EQ(lexer.next().kind(), TokenKind::PointerAccess);
}

TEST(LexerTests, MinusAlone) {
    Context context;
    auto lexer = makeLexer(context, "- 1");
    EXPECT_EQ(lexer.next().kind(), TokenKind::Minus);
}

// ------------------------------------
// Token range, string, and lexeme
// ------------------------------------

TEST(LexerTests, TokenRangeCoversLexeme) {
    Context context;
    auto lexer = makeLexer(context, "  hello  ");
    auto tok = lexer.next();
    EXPECT_EQ(tok.kind(), TokenKind::Identifier);
    EXPECT_EQ(tok.lexeme(), "hello");
}

TEST(LexerTests, TokenRangeForMultiCharOperator) {
    Context context;
    auto lexer = makeLexer(context, "<=");
    auto tok = lexer.next();
    EXPECT_EQ(tok.lexeme(), "<=");
}

TEST(LexerTests, StringMethodForIdentifier) {
    Context context;
    auto lexer = makeLexer(context, "myVar");
    auto tok = lexer.next();
    EXPECT_EQ(tok.string(), "MYVAR");
}

TEST(LexerTests, StringMethodForStringLiteral) {
    Context context;
    auto lexer = makeLexer(context, "\"test\"");
    auto tok = lexer.next();
    EXPECT_EQ(tok.string(), "test");
}

TEST(LexerTests, StringMethodForKeyword) {
    Context context;
    auto lexer = makeLexer(context, "IF");
    auto tok = lexer.next();
    EXPECT_EQ(tok.string(), "IF");
}

TEST(LexerTests, StringMethodForIntegerReturnsLexeme) {
    Context context;
    auto lexer = makeLexer(context, "42");
    auto tok = lexer.next();
    EXPECT_EQ(tok.string(), "42");
}

TEST(LexerTests, LexemeForStringLiteralIncludesQuotes) {
    Context context;
    auto lexer = makeLexer(context, "\"hello\"");
    auto tok = lexer.next();
    EXPECT_EQ(tok.lexeme(), "\"hello\"");
    EXPECT_EQ(tok.string(), "hello");
}

// ------------------------------------
// Identifiers and keywords
// ------------------------------------

TEST(LexerTests, IdentifierIsCaseInsensitive) {
    Context context;
    auto lexer = makeLexer(context, "iF");
    EXPECT_EQ(lexer.next().kind(), TokenKind::If);
}

TEST(LexerTests, UnderscorePrefixedIdentifier) {
    Context context;
    auto lexer = makeLexer(context, "_foo");
    auto tok = lexer.next();
    EXPECT_EQ(tok.kind(), TokenKind::Identifier);
    EXPECT_EQ(tok.string(), "_FOO");
}

// ------------------------------------
// Peek
// ------------------------------------

TEST(LexerTests, PeekDoesNotConsumeToken) {
    Context context;
    auto lexer = makeLexer(context, "42");
    auto peeked = lexer.peek();
    auto next = lexer.next();
    EXPECT_EQ(peeked.kind(), next.kind());
    EXPECT_EQ(peeked.kind(), TokenKind::IntegerLiteral);
}

// ------------------------------------
// End of file
// ------------------------------------

TEST(LexerTests, EmptyInputProducesEndOfFile) {
    Context context;
    auto lexer = makeLexer(context, "");
    EXPECT_EQ(lexer.next().kind(), TokenKind::EndOfFile);
}
