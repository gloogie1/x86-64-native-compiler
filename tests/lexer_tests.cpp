#include "compiler/lexer/lexer.hpp"
#include "compiler/lexer/token.hpp"
#include "compiler/source/source_file.hpp"
#include <gtest/gtest.h>
#include <variant>
#include <array>


TEST(LexerTest, EmptySourceProducesEof) {
    const compiler::SourceFile source{"test.xc", ""};
    compiler::Lexer lexer{source};

    const compiler::LexResult result = lexer.next();

    ASSERT_TRUE(std::holds_alternative<compiler::Token>(result));

    const auto& token = std::get<compiler::Token>(result);

    EXPECT_EQ(token.kind, compiler::TokenKind::Eof);
    EXPECT_EQ(token.span.begin, 0);
    EXPECT_EQ(token.span.end, 0);
}

TEST(LexerTest, WhiteSpaceTriviaProducesEOF) {
    const compiler::SourceFile source{"test.xc", " \t\n\r"};
    compiler::Lexer lexer{source};

    const compiler::LexResult result = lexer.next();

    ASSERT_TRUE(std::holds_alternative<compiler::Token>(result));

    const auto& token = std::get<compiler::Token>(result);

    EXPECT_EQ(token.kind, compiler::TokenKind::Eof);
    EXPECT_EQ(token.span.begin, 4);
    EXPECT_EQ(token.span.end, 4);
}


TEST(LexerTest, LineCommentAtEofProducesEof) {
    const compiler::SourceFile source{"test.xc", "// hello world"};
    compiler::Lexer lexer{source};

    const compiler::LexResult result = lexer.next();

    ASSERT_TRUE(std::holds_alternative<compiler::Token>(result));

    const auto& token = std::get<compiler::Token>(result);

    EXPECT_EQ(token.kind, compiler::TokenKind::Eof);
    EXPECT_EQ(token.span.begin, 14);
    EXPECT_EQ(token.span.end, 14);
}

TEST(LexerTest, CommentEOFProducesEOF) {
    const compiler::SourceFile source{"test.xc", "// hello world\n"};
    compiler::Lexer lexer{source};

    const compiler::LexResult result = lexer.next();

    ASSERT_TRUE(std::holds_alternative<compiler::Token>(result));

    const auto& token = std::get<compiler::Token>(result);

    EXPECT_EQ(token.kind, compiler::TokenKind::Eof);
    EXPECT_EQ(token.span.begin, 15);
    EXPECT_EQ(token.span.end, 15);
}

TEST(LexerTest, SingleIdentifierLetters) {
    const compiler::SourceFile source{"test.xc", "hello"};
    compiler::Lexer lexer{source};

    const compiler::LexResult result = lexer.next();

    ASSERT_TRUE(std::holds_alternative<compiler::Token>(result));

    const auto& token = std::get<compiler::Token>(result);

    EXPECT_EQ(token.kind, compiler::TokenKind::Identifier);
    EXPECT_EQ(token.span.begin, 0);
    EXPECT_EQ(token.span.end, 5);
}


TEST(LexerTest, SingleIdentifierWithUnderscoreNumbers) {
    const compiler::SourceFile source{"test.xc", "hello_123"};
    compiler::Lexer lexer{source};

    const compiler::LexResult result = lexer.next();

    ASSERT_TRUE(std::holds_alternative<compiler::Token>(result));

    const auto& token = std::get<compiler::Token>(result);

    EXPECT_EQ(token.kind, compiler::TokenKind::Identifier);
    EXPECT_EQ(token.span.begin, 0);
    EXPECT_EQ(token.span.end, 9);
}

TEST(LexerTest, RecognizesKeywords) {
    const compiler::SourceFile source{"test.xc", "fn let if else while return true false i64 bool void"};
    const std::array expected{
        compiler::TokenKind::KwFn,
        compiler::TokenKind::KwLet,
        compiler::TokenKind::KwIf,
        compiler::TokenKind::KwElse,
        compiler::TokenKind::KwWhile,
        compiler::TokenKind::KwReturn,
        compiler::TokenKind::KwTrue,
        compiler::TokenKind::KwFalse,
        compiler::TokenKind::KwI64,
        compiler::TokenKind::KwBool,
        compiler::TokenKind::KwVoid,
        compiler::TokenKind::Eof
    };

    compiler::Lexer lexer{source};

    for (const compiler::TokenKind expected_kind : expected) {
        compiler::LexResult result = lexer.next();
        ASSERT_TRUE(std::holds_alternative<compiler::Token>(result));
        const auto& token = std::get<compiler::Token>(result);
        EXPECT_EQ(token.kind, expected_kind);
    }
}

TEST(LexerTest, IdentifiersSimilarToKeywords) {
    const compiler::SourceFile source{"test.xc", "iffy whileThing trueValue"};
    const std::array expected{
        compiler::TokenKind::Identifier,
        compiler::TokenKind::Identifier,
        compiler::TokenKind::Identifier,
        compiler::TokenKind::Eof
    };

    compiler::Lexer lexer{source};

    for (const compiler::TokenKind expected_kind : expected) {
        compiler::LexResult result = lexer.next();
        ASSERT_TRUE(std::holds_alternative<compiler::Token>(result));
        const auto& token = std::get<compiler::Token>(result);
        EXPECT_EQ(token.kind, expected_kind);
    }
}

TEST(LexerTest, RecognizesInteger) {
    const compiler::SourceFile source{"test.xc", "12345"};

    compiler::Lexer lexer{source};

    const compiler::LexResult result = lexer.next();

    ASSERT_TRUE(std::holds_alternative<compiler::Token>(result));

    const auto& token = std::get<compiler::Token>(result);

    EXPECT_EQ(token.kind, compiler::TokenKind::IntegerLiteral);
    EXPECT_EQ(token.span.begin, 0);
    EXPECT_EQ(token.span.end, 5);
}


TEST(LexerTest, RecognizesSingleCharacterSymbols) {
    const compiler::SourceFile source{"test.xc", "+ - * / % ! = < > ( ) { } : ; ,"};
    const std::array expected{
        compiler::TokenKind::Plus,
        compiler::TokenKind::Minus,
        compiler::TokenKind::Star,
        compiler::TokenKind::Slash,
        compiler::TokenKind::Percent,
        compiler::TokenKind::Bang,
        compiler::TokenKind::Equal,
        compiler::TokenKind::Less,
        compiler::TokenKind::Greater,
        compiler::TokenKind::LeftParen,
        compiler::TokenKind::RightParen,
        compiler::TokenKind::LeftBrace,
        compiler::TokenKind::RightBrace,
        compiler::TokenKind::Colon,
        compiler::TokenKind::Semicolon,
        compiler::TokenKind::Comma,
        compiler::TokenKind::Eof
    };

    compiler::Lexer lexer{source};

    for (const compiler::TokenKind expected_kind : expected) {
        compiler::LexResult result = lexer.next();
        ASSERT_TRUE(std::holds_alternative<compiler::Token>(result));
        const auto& token = std::get<compiler::Token>(result);
        EXPECT_EQ(token.kind, expected_kind);
    }
}

TEST(LexerTest, RecognizesDoubleCharacterSymbols) {
    const compiler::SourceFile source{"test.xc","== != <= >= && || ->"};
    const std::array expected{
        compiler::TokenKind::EqualEqual,
        compiler::TokenKind::BangEqual,
        compiler::TokenKind::LessEqual,
        compiler::TokenKind::GreaterEqual,
        compiler::TokenKind::AmpAmp,
        compiler::TokenKind::PipePipe,
        compiler::TokenKind::Arrow,
        compiler::TokenKind::Eof
    };

    compiler::Lexer lexer{source};

    for (const compiler::TokenKind expected_kind : expected) {
        compiler::LexResult result = lexer.next();
        ASSERT_TRUE(std::holds_alternative<compiler::Token>(result));
        const auto& token = std::get<compiler::Token>(result);
        EXPECT_EQ(token.kind, expected_kind);
    }
}

TEST(LexerTest, MixedTokenTypes) {
    const compiler::SourceFile source{"test.xc","fn main() -> i64 { let x = 42; return x + 1; }"};
    const std::array expected{
        compiler::TokenKind::KwFn,
        compiler::TokenKind::Identifier,
        compiler::TokenKind::LeftParen,
        compiler::TokenKind::RightParen,
        compiler::TokenKind::Arrow,
        compiler::TokenKind::KwI64,
        compiler::TokenKind::LeftBrace,
        compiler::TokenKind::KwLet,
        compiler::TokenKind::Identifier,
        compiler::TokenKind::Equal,
        compiler::TokenKind::IntegerLiteral,
        compiler::TokenKind::Semicolon,
        compiler::TokenKind::KwReturn,
        compiler::TokenKind::Identifier,
        compiler::TokenKind::Plus,
        compiler::TokenKind::IntegerLiteral,
        compiler::TokenKind::Semicolon,
        compiler::TokenKind::RightBrace,
        compiler::TokenKind::Eof
    };

    compiler::Lexer lexer{source};

    for (const compiler::TokenKind expected_kind : expected) {
        compiler::LexResult result = lexer.next();
        ASSERT_TRUE(std::holds_alternative<compiler::Token>(result));
        const auto& token = std::get<compiler::Token>(result);
        EXPECT_EQ(token.kind, expected_kind);
    }
}

TEST(LexerTest, TriviaMixedWithTokens) {
    const compiler::SourceFile source{"test.xc","let\tfoo\n=\r123 ;"};
    compiler::Lexer lexer{source};
    const compiler::LexResult result1 = lexer.next();

    ASSERT_TRUE(std::holds_alternative<compiler::Token>(result1));

    const auto& token1 = std::get<compiler::Token>(result1);

    EXPECT_EQ(token1.kind, compiler::TokenKind::KwLet);
    EXPECT_EQ(token1.span.begin, 0);
    EXPECT_EQ(token1.span.end, 3);

    const compiler::LexResult result2 = lexer.next();

    ASSERT_TRUE(std::holds_alternative<compiler::Token>(result2));

    const auto& token2 = std::get<compiler::Token>(result2);

    EXPECT_EQ(token2.kind, compiler::TokenKind::Identifier);
    EXPECT_EQ(token2.span.begin, 4);
    EXPECT_EQ(token2.span.end, 7);

    const compiler::LexResult result3 = lexer.next();

    ASSERT_TRUE(std::holds_alternative<compiler::Token>(result3));

    const auto& token3 = std::get<compiler::Token>(result3);

    EXPECT_EQ(token3.kind, compiler::TokenKind::Equal);
    EXPECT_EQ(token3.span.begin, 8);
    EXPECT_EQ(token3.span.end, 9);

    const compiler::LexResult result4 = lexer.next();

    ASSERT_TRUE(std::holds_alternative<compiler::Token>(result4));

    const auto& token4 = std::get<compiler::Token>(result4);

    EXPECT_EQ(token4.kind, compiler::TokenKind::IntegerLiteral);
    EXPECT_EQ(token4.span.begin, 10);
    EXPECT_EQ(token4.span.end, 13);
    
    const compiler::LexResult result5 = lexer.next();

    ASSERT_TRUE(std::holds_alternative<compiler::Token>(result5));

    const auto& token5 = std::get<compiler::Token>(result5);

    EXPECT_EQ(token5.kind, compiler::TokenKind::Semicolon);
    EXPECT_EQ(token5.span.begin, 14);
    EXPECT_EQ(token5.span.end, 15);

    const compiler::LexResult result6 = lexer.next();

    ASSERT_TRUE(std::holds_alternative<compiler::Token>(result6));

    const auto& token6 = std::get<compiler::Token>(result6);

    EXPECT_EQ(token6.kind, compiler::TokenKind::Eof);
    EXPECT_EQ(token6.span.begin, 15);
    EXPECT_EQ(token6.span.end, 15);
}


TEST(LexerTest, CommentBetweenTokens) {
    const compiler::SourceFile source{"test.xc","let // comment\nreturn;"};
    compiler::Lexer lexer{source};
    const compiler::LexResult result1 = lexer.next();

    ASSERT_TRUE(std::holds_alternative<compiler::Token>(result1));

    const auto& token1 = std::get<compiler::Token>(result1);

    EXPECT_EQ(token1.kind, compiler::TokenKind::KwLet);
    EXPECT_EQ(token1.span.begin, 0);
    EXPECT_EQ(token1.span.end, 3);

    const compiler::LexResult result2 = lexer.next();

    ASSERT_TRUE(std::holds_alternative<compiler::Token>(result2));

    const auto& token2 = std::get<compiler::Token>(result2);

    EXPECT_EQ(token2.kind, compiler::TokenKind::KwReturn);
    EXPECT_EQ(token2.span.begin, 15);
    EXPECT_EQ(token2.span.end, 21);
}


TEST(LexerTest, LoneAmpersandProducesError) {
    const compiler::SourceFile source{"test.xc", "&"};
    compiler::Lexer lexer{source};

    const compiler::LexResult result = lexer.next();

    ASSERT_TRUE(std::holds_alternative<compiler::LexError>(result));

    const auto& error = std::get<compiler::LexError>(result);

    EXPECT_EQ(error.span.begin, 0);
    EXPECT_EQ(error.span.end, 1);
}

TEST(LexerTest, LonePipeProducesError) {
    const compiler::SourceFile source{"test.xc", "|"};
    compiler::Lexer lexer{source};

    const compiler::LexResult result = lexer.next();

    ASSERT_TRUE(std::holds_alternative<compiler::LexError>(result));

    const auto& error = std::get<compiler::LexError>(result);

    EXPECT_EQ(error.span.begin, 0);
    EXPECT_EQ(error.span.end, 1);
}