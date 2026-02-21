#include "pch.hpp"
#include <gtest/gtest.h>
#include "Driver/Context.hpp"
#include "Lexer/Lexer.hpp"
#include "Lexer/TokenKind.hpp"
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

/**
 * Unwrap a DiagResult<Token>, failing the test on error.
 */
auto tok(DiagResult<Token> result) -> Token {
    EXPECT_TRUE(result.has_value()) << "expected a valid token, got an error";
    return result.value_or(Token {});
}

} // namespace

// ------------------------------------
// Comments
// ------------------------------------

TEST(LexerTests, Comments) {
    Context context;
    // single-line
    EXPECT_EQ(tok(makeLexer(context, "' comment\n42").next()).kind(), TokenKind::IntegerLiteral);
    // multiline (nested)
    EXPECT_EQ(tok(makeLexer(context, "/' outer /' inner '/ '/ 42").next()).kind(), TokenKind::IntegerLiteral);
    // unclosed multiline reaches EOF
    EXPECT_EQ(tok(makeLexer(context, "/' unclosed").next()).kind(), TokenKind::EndOfFile);
}

// ------------------------------------
// Newlines and statements
// ------------------------------------

TEST(LexerTests, NewlinesAndStatements) {
    Context context;
    // leading newlines are skipped
    EXPECT_EQ(tok(makeLexer(context, "\n\n\n42").next()).kind(), TokenKind::IntegerLiteral);
    // newline produces EndOfStmt, consecutive newlines produce only one
    auto lexer = makeLexer(context, "42\n\n\n43");
    EXPECT_EQ(tok(lexer.next()).kind(), TokenKind::IntegerLiteral);
    EXPECT_EQ(tok(lexer.next()).kind(), TokenKind::EndOfStmt);
    EXPECT_EQ(tok(lexer.next()).kind(), TokenKind::IntegerLiteral);
    // CRLF
    auto lexer2 = makeLexer(context, "42\r\n43");
    EXPECT_EQ(tok(lexer2.next()).kind(), TokenKind::IntegerLiteral);
    EXPECT_EQ(tok(lexer2.next()).kind(), TokenKind::EndOfStmt);
    EXPECT_EQ(tok(lexer2.next()).kind(), TokenKind::IntegerLiteral);
}

TEST(LexerTests, LineContinuation) {
    Context context;
    auto lexer = makeLexer(context, "42 _\n+ 43");
    EXPECT_EQ(tok(lexer.next()).kind(), TokenKind::IntegerLiteral);
    EXPECT_EQ(tok(lexer.next()).kind(), TokenKind::Plus);
    EXPECT_EQ(tok(lexer.next()).kind(), TokenKind::IntegerLiteral);
}

// ------------------------------------
// Literals
// ------------------------------------

TEST(LexerTests, BooleanAndNullLiterals) {
    Context context;
    auto tr = tok(makeLexer(context, "True").next());
    EXPECT_EQ(tr.kind(), TokenKind::BooleanLiteral);
    EXPECT_EQ(tr.getValue().get<bool>(), true);
    auto fl = tok(makeLexer(context, "FALSE").next());
    EXPECT_EQ(fl.kind(), TokenKind::BooleanLiteral);
    EXPECT_EQ(fl.getValue().get<bool>(), false);
    EXPECT_EQ(tok(makeLexer(context, "Null").next()).kind(), TokenKind::NullLiteral);
}

TEST(LexerTests, StringLiterals) {
    Context context;
    // basic string
    auto t = tok(makeLexer(context, "\"hello world\"").next());
    EXPECT_EQ(t.kind(), TokenKind::StringLiteral);
    EXPECT_EQ(t.getValue().get<llvm::StringRef>(), "hello world");
    // unclosed string is an error
    EXPECT_FALSE(makeLexer(context, "\"unclosed").next().has_value());
}

TEST(LexerTests, StringEscapeSequences) {
    Context context;
    // all valid escapes
    auto t = tok(makeLexer(context, R"("\a\b\f\n\r\t\v\\\'\"\0")").next());
    EXPECT_EQ(t.kind(), TokenKind::StringLiteral);
    EXPECT_EQ(t.getValue().get<llvm::StringRef>(), R"(\a\b\f\n\r\t\v\\\'\"\0)");
    // escaped quote doesn't terminate
    EXPECT_EQ(tok(makeLexer(context, R"("say \"hi\"")").next()).kind(), TokenKind::StringLiteral);
    // invalid escapes
    EXPECT_FALSE(makeLexer(context, R"("bad\x")").next().has_value());
    EXPECT_FALSE(makeLexer(context, "\"trailing\\").next().has_value());
}

TEST(LexerTests, StringWithInvisibleChars) {
    Context context;
    EXPECT_FALSE(makeLexer(context, std::string("\"a\x01z\"")).next().has_value());
    EXPECT_FALSE(makeLexer(context, std::string("\"a\tz\"")).next().has_value());
}

TEST(LexerTests, NumberLiterals) {
    Context context;
    // integer
    auto integer = tok(makeLexer(context, "12345").next());
    EXPECT_EQ(integer.kind(), TokenKind::IntegerLiteral);
    EXPECT_EQ(integer.getValue().get<std::uint64_t>(), 12345U);
    // float
    auto fp = tok(makeLexer(context, "3.14").next());
    EXPECT_EQ(fp.kind(), TokenKind::FloatLiteral);
    EXPECT_DOUBLE_EQ(fp.getValue().get<double>(), 3.14);
    // float starting with dot
    auto dot = tok(makeLexer(context, ".5").next());
    EXPECT_EQ(dot.kind(), TokenKind::FloatLiteral);
    EXPECT_DOUBLE_EQ(dot.getValue().get<double>(), 0.5);
    // trailing alpha is an error
    EXPECT_FALSE(makeLexer(context, "123abc").next().has_value());
}

// ------------------------------------
// Operators and symbols
// ------------------------------------

TEST(LexerTests, SingleCharOperators) {
    Context context;
    EXPECT_EQ(tok(makeLexer(context, "=").next()).kind(), TokenKind::Assign);
    EXPECT_EQ(tok(makeLexer(context, "+").next()).kind(), TokenKind::Plus);
    EXPECT_EQ(tok(makeLexer(context, "*").next()).kind(), TokenKind::Multiply);
    EXPECT_EQ(tok(makeLexer(context, "@").next()).kind(), TokenKind::AddressOf);
    EXPECT_EQ(tok(makeLexer(context, ",").next()).kind(), TokenKind::Comma);
    EXPECT_EQ(tok(makeLexer(context, "(").next()).kind(), TokenKind::ParenOpen);
    EXPECT_EQ(tok(makeLexer(context, ")").next()).kind(), TokenKind::ParenClose);
    EXPECT_EQ(tok(makeLexer(context, "[").next()).kind(), TokenKind::BracketOpen);
    EXPECT_EQ(tok(makeLexer(context, "]").next()).kind(), TokenKind::BracketClose);
}

TEST(LexerTests, MultiCharOperators) {
    Context context;
    EXPECT_EQ(tok(makeLexer(context, "<>").next()).kind(), TokenKind::NotEqual);
    EXPECT_EQ(tok(makeLexer(context, "<=").next()).kind(), TokenKind::LessOrEqual);
    EXPECT_EQ(tok(makeLexer(context, ">=").next()).kind(), TokenKind::GreaterOrEqual);
    EXPECT_EQ(tok(makeLexer(context, "->").next()).kind(), TokenKind::PointerAccess);
    EXPECT_EQ(tok(makeLexer(context, "...").next()).kind(), TokenKind::Ellipsis);
    // single < and > when not followed by matching char
    EXPECT_EQ(tok(makeLexer(context, "< 1").next()).kind(), TokenKind::LessThan);
    EXPECT_EQ(tok(makeLexer(context, "> 1").next()).kind(), TokenKind::GreaterThan);
    EXPECT_EQ(tok(makeLexer(context, "- 1").next()).kind(), TokenKind::Minus);
}

TEST(LexerTests, DotVariants) {
    Context context;
    // member access
    auto lexer = makeLexer(context, "a.b");
    EXPECT_EQ(tok(lexer.next()).kind(), TokenKind::Identifier);
    EXPECT_EQ(tok(lexer.next()).kind(), TokenKind::MemberAccess);
    EXPECT_EQ(tok(lexer.next()).kind(), TokenKind::Identifier);
    // two dots is an error
    EXPECT_FALSE(makeLexer(context, "..").next().has_value());
}

// ------------------------------------
// Token string and lexeme
// ------------------------------------

TEST(LexerTests, TokenStringAndLexeme) {
    Context context;
    // identifier: string() uppercases, lexeme() preserves source
    auto id = tok(makeLexer(context, "  myVar  ").next());
    EXPECT_EQ(id.string(), "MYVAR");
    EXPECT_EQ(id.lexeme(), "myVar");
    // string literal: string() returns content, lexeme() includes quotes
    auto str = tok(makeLexer(context, "\"hello\"").next());
    EXPECT_EQ(str.string(), "hello");
    EXPECT_EQ(str.lexeme(), "\"hello\"");
    // keyword: string() returns kind name
    EXPECT_EQ(tok(makeLexer(context, "IF").next()).string(), "IF");
    // number: string() returns lexeme
    EXPECT_EQ(tok(makeLexer(context, "42").next()).string(), "42");
    // multi-char operator lexeme
    EXPECT_EQ(tok(makeLexer(context, "<=").next()).lexeme(), "<=");
}

// ------------------------------------
// Identifiers and keywords
// ------------------------------------

TEST(LexerTests, Identifiers) {
    Context context;
    // case insensitive keyword match
    EXPECT_EQ(tok(makeLexer(context, "iF").next()).kind(), TokenKind::If);
    // underscore-prefixed identifier
    auto t = tok(makeLexer(context, "_foo").next());
    EXPECT_EQ(t.kind(), TokenKind::Identifier);
    EXPECT_EQ(t.string(), "_FOO");
}

// ------------------------------------
// Peek and EOF
// ------------------------------------

TEST(LexerTests, PeekDoesNotConsumeToken) {
    Context context;
    auto lexer = makeLexer(context, "42");
    auto peeked = tok(lexer.peek());
    auto next = tok(lexer.next());
    EXPECT_EQ(peeked.kind(), next.kind());
    EXPECT_EQ(peeked.kind(), TokenKind::IntegerLiteral);
}

TEST(LexerTests, EmptyInputProducesEndOfFile) {
    Context context;
    EXPECT_EQ(tok(makeLexer(context, "").next()).kind(), TokenKind::EndOfFile);
}
