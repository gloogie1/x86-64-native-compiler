#include "compiler/lexer/lexer.hpp"
#include "compiler/parser/parser.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <utility>

std::vector<compiler::Token> lex_source(const compiler::SourceFile& source) {
    compiler::Lexer lexer{source};
    std::vector<compiler::Token> tokens;

    while (true) {
        compiler::LexResult lex_result = lexer.next();

        if (!std::holds_alternative<compiler::Token>(lex_result)) {
            ADD_FAILURE() << "lexer produced an error";
            return {};
        }

        compiler::Token token =
            std::get<compiler::Token>(lex_result);

        tokens.push_back(token);

        if (token.kind == compiler::TokenKind::Eof) {
            break;
        }
    }

    return tokens;
}

compiler::ExprParseResult parse_test_expression(std::string_view text) {
    compiler::SourceFile source{"test.xc", std::string{text}};
    std::vector<compiler::Token> tokens = lex_source(source);

    compiler::Parser parser{source, std::move(tokens)};

    return parser.parse_expression();
}

compiler::ProgramParseResult parse_test_program(std::string_view text) {
    compiler::SourceFile source{"test.xc", std::string{text}};
    std::vector<compiler::Token> tokens = lex_source(source);
    compiler::Parser parser{source, std::move(tokens)};
    return parser.parse_program();
}

TEST(ParserTest, IntegerLiteral) {
    auto result = parse_test_expression("42");

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr = std::get<std::unique_ptr<compiler::Expr>>(result);

    const auto* integer = 
        dynamic_cast<const compiler::IntegerLiteralExpr*>(expr.get());

    ASSERT_NE(integer, nullptr);
    EXPECT_EQ(integer->value, 42);
    EXPECT_EQ(integer->span.begin, 0);
    EXPECT_EQ(integer->span.end, 2);
}

TEST(ParserTest, BooleanTrue){
    auto result = parse_test_expression("true");

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr = std::get<std::unique_ptr<compiler::Expr>>(result);
    const auto* bool_true = 
        dynamic_cast<const compiler::BoolLiteralExpr*>(expr.get());

    ASSERT_NE(bool_true, nullptr);
    EXPECT_EQ(bool_true->value, true);
    EXPECT_EQ(bool_true->span.begin, 0);
    EXPECT_EQ(bool_true->span.end, 4);
}

TEST(ParserTest, BooleanFalse){
    auto result = parse_test_expression("false");

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr = std::get<std::unique_ptr<compiler::Expr>>(result);
    const auto* bool_false = 
        dynamic_cast<const compiler::BoolLiteralExpr*>(expr.get());

    ASSERT_NE(bool_false, nullptr);
    EXPECT_EQ(bool_false->value, false);
    EXPECT_EQ(bool_false->span.begin, 0);
    EXPECT_EQ(bool_false->span.end, 5);
}

TEST(ParserTest, Identifier){
    auto result = parse_test_expression("counter");

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr = std::get<std::unique_ptr<compiler::Expr>>(result);
    const auto* var = 
        dynamic_cast<const compiler::VariableExpr*>(expr.get());

    ASSERT_NE(var, nullptr);
    EXPECT_EQ(var->name, "counter");
    EXPECT_EQ(var->span.begin, 0);
    EXPECT_EQ(var->span.end, 7);
}

TEST(ParserTest, ParenthesizedExpression){
    auto result = parse_test_expression("(42)");

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr = std::get<std::unique_ptr<compiler::Expr>>(result);

    const auto* integer = 
        dynamic_cast<const compiler::IntegerLiteralExpr*>(expr.get());

    ASSERT_NE(integer, nullptr);
    EXPECT_EQ(integer->value, 42);
    EXPECT_EQ(integer->span.begin, 1);
    EXPECT_EQ(integer->span.end, 3);
}

TEST(ParserTest, NestedParentheses){
    auto result = parse_test_expression("((42))");
    
    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr = std::get<std::unique_ptr<compiler::Expr>>(result);

    const auto* integer = 
        dynamic_cast<const compiler::IntegerLiteralExpr*>(expr.get());

    ASSERT_NE(integer, nullptr);
    EXPECT_EQ(integer->value, 42);
    EXPECT_EQ(integer->span.begin, 2);
    EXPECT_EQ(integer->span.end, 4);
}

TEST(ParserTest, MissingRightParenthesis){
    auto result = parse_test_expression("(42");

    ASSERT_TRUE(
        std::holds_alternative<compiler::ParseError>(result)
    );

    const auto& error =
        std::get<compiler::ParseError>(result);

    EXPECT_EQ(error.message, "expected ')' after expression");
    EXPECT_EQ(error.span.begin, 3);
    EXPECT_EQ(error.span.end, 3);
}

TEST(ParserTest, InvalidPrimaryToken) {
    auto result = parse_test_expression("+");

    ASSERT_TRUE(
        std::holds_alternative<compiler::ParseError>(result)
    );

    const auto& error =
        std::get<compiler::ParseError>(result);

    EXPECT_EQ(error.message, "expected expression");
    EXPECT_EQ(error.span.begin, 0);
    EXPECT_EQ(error.span.end, 1);
}

TEST(ParserTest, EmptySource) {
    auto result = parse_test_expression("");

    ASSERT_TRUE(
        std::holds_alternative<compiler::ParseError>(result)
    );

    const auto& error =
        std::get<compiler::ParseError>(result);

    EXPECT_EQ(error.message, "expected expression");
    EXPECT_EQ(error.span.begin, 0);
    EXPECT_EQ(error.span.end, 0);
}

TEST(ParserTest, IntegerOverflow) {
    constexpr std::string_view input =
        "999999999999999999999999999999999";

    auto result = parse_test_expression(input);

    ASSERT_TRUE(
        std::holds_alternative<compiler::ParseError>(result)
    );

    const auto& error =
        std::get<compiler::ParseError>(result);

    EXPECT_EQ(
        error.message,
        "integer literal is out of range for i64"
    );
    EXPECT_EQ(error.span.begin, 0);
    EXPECT_EQ(error.span.end, input.size());
}


TEST(ParserTest, UnaryNegation) {
    auto result = parse_test_expression("-42");

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr = std::get<std::unique_ptr<compiler::Expr>>(result);

    const auto* unary = 
        dynamic_cast<const compiler::UnaryExpr*>(expr.get());

    ASSERT_NE(unary, nullptr);
    EXPECT_EQ(unary->op, compiler::UnaryOp::Negate);
    EXPECT_EQ(unary->span.begin, 0);
    EXPECT_EQ(unary->span.end, 3);

    const auto* integer =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            unary->operand.get()
        );

    ASSERT_NE(integer, nullptr);

    EXPECT_EQ(integer->value, 42);
    EXPECT_EQ(integer->span.begin, 1);
    EXPECT_EQ(integer->span.end, 3);
}

TEST(ParserTest, UnaryLogicalNot) {
    auto result = parse_test_expression("!true");

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr = std::get<std::unique_ptr<compiler::Expr>>(result);

    const auto* unary =
        dynamic_cast<const compiler::UnaryExpr*>(expr.get());

    ASSERT_NE(unary, nullptr);
    EXPECT_EQ(unary->op, compiler::UnaryOp::LogicalNot);
    EXPECT_EQ(unary->span.begin, 0);
    EXPECT_EQ(unary->span.end, 5);

    const auto* boolean =
        dynamic_cast<const compiler::BoolLiteralExpr*>(
            unary->operand.get()
        );

    ASSERT_NE(boolean, nullptr);

    EXPECT_EQ(boolean->value, true);
    EXPECT_EQ(boolean->span.begin, 1);
    EXPECT_EQ(boolean->span.end, 5);
}

TEST(ParserTest, NestedUnary) {
    auto result = parse_test_expression("!!false");

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr = std::get<std::unique_ptr<compiler::Expr>>(result);

    const auto* outer =
        dynamic_cast<const compiler::UnaryExpr*>(expr.get());

    ASSERT_NE(outer, nullptr);
    EXPECT_EQ(outer->op, compiler::UnaryOp::LogicalNot);
    EXPECT_EQ(outer->span.begin, 0);
    EXPECT_EQ(outer->span.end, 7);

    const auto* inner =
        dynamic_cast<const compiler::UnaryExpr*>(
            outer->operand.get()
        );

    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->op, compiler::UnaryOp::LogicalNot);
    EXPECT_EQ(inner->span.begin, 1);
    EXPECT_EQ(inner->span.end, 7);  
    
    const auto* boolean =
        dynamic_cast<const compiler::BoolLiteralExpr*>(
            inner->operand.get()
        );

    ASSERT_NE(boolean, nullptr);

    EXPECT_EQ(boolean->value, false);
    EXPECT_EQ(boolean->span.begin, 2);
    EXPECT_EQ(boolean->span.end, 7);
}

TEST(ParserTest, Multiplication) {
    auto result = parse_test_expression("2 * 3");

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr = std::get<std::unique_ptr<compiler::Expr>>(result);

    const auto* binary = 
        dynamic_cast<const compiler::BinaryExpr*>(expr.get());

    ASSERT_NE(binary, nullptr);
    EXPECT_EQ(binary->op, compiler::BinaryOp::Multiply);
    EXPECT_EQ(binary->span.begin, 0);
    EXPECT_EQ(binary->span.end, 5);

    const auto* left =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            binary->left.get()
        );

    const auto* right =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            binary->right.get()
        );

    ASSERT_NE(left, nullptr);

    EXPECT_EQ(left->value, 2);
    EXPECT_EQ(left->span.begin, 0);
    EXPECT_EQ(left->span.end, 1);

    ASSERT_NE(right, nullptr);

    EXPECT_EQ(right->value, 3);
    EXPECT_EQ(right->span.begin, 4);
    EXPECT_EQ(right->span.end, 5);
}

TEST(ParserTest, Addition) {
    auto result = parse_test_expression("1 + 2");

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr = std::get<std::unique_ptr<compiler::Expr>>(result);

    const auto* binary =
        dynamic_cast<const compiler::BinaryExpr*>(expr.get());

    ASSERT_NE(binary, nullptr);
    EXPECT_EQ(binary->op, compiler::BinaryOp::Add);
    EXPECT_EQ(binary->span.begin, 0);
    EXPECT_EQ(binary->span.end, 5);

    const auto* left =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            binary->left.get()
        );

    const auto* right =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            binary->right.get()
        );

    ASSERT_NE(left, nullptr);
    EXPECT_EQ(left->value, 1);
    EXPECT_EQ(left->span.begin, 0);
    EXPECT_EQ(left->span.end, 1);

    ASSERT_NE(right, nullptr);
    EXPECT_EQ(right->value, 2);
    EXPECT_EQ(right->span.begin, 4);
    EXPECT_EQ(right->span.end, 5);
}

TEST(ParserTest, Comparison) {
    auto result = parse_test_expression("1 < 2");

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr = std::get<std::unique_ptr<compiler::Expr>>(result);

    const auto* binary =
        dynamic_cast<const compiler::BinaryExpr*>(expr.get());

    ASSERT_NE(binary, nullptr);
    EXPECT_EQ(binary->op, compiler::BinaryOp::Less);
    EXPECT_EQ(binary->span.begin, 0);
    EXPECT_EQ(binary->span.end, 5);

    const auto* left =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            binary->left.get()
        );

    const auto* right =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            binary->right.get()
        );

    ASSERT_NE(left, nullptr);
    EXPECT_EQ(left->value, 1);

    ASSERT_NE(right, nullptr);
    EXPECT_EQ(right->value, 2);
}

TEST(ParserTest, Equality) {
    auto result = parse_test_expression("1 == 2");

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr = std::get<std::unique_ptr<compiler::Expr>>(result);

    const auto* binary =
        dynamic_cast<const compiler::BinaryExpr*>(expr.get());

    ASSERT_NE(binary, nullptr);
    EXPECT_EQ(binary->op, compiler::BinaryOp::Equal);
    EXPECT_EQ(binary->span.begin, 0);
    EXPECT_EQ(binary->span.end, 6);

    const auto* left =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            binary->left.get()
        );

    const auto* right =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            binary->right.get()
        );

    ASSERT_NE(left, nullptr);
    EXPECT_EQ(left->value, 1);

    ASSERT_NE(right, nullptr);
    EXPECT_EQ(right->value, 2);
}

TEST(ParserTest, LogicalAnd) {
    auto result = parse_test_expression("true && false");

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr = std::get<std::unique_ptr<compiler::Expr>>(result);

    const auto* binary =
        dynamic_cast<const compiler::BinaryExpr*>(expr.get());

    ASSERT_NE(binary, nullptr);
    EXPECT_EQ(binary->op, compiler::BinaryOp::LogicalAnd);
    EXPECT_EQ(binary->span.begin, 0);
    EXPECT_EQ(binary->span.end, 13);

    const auto* left =
        dynamic_cast<const compiler::BoolLiteralExpr*>(
            binary->left.get()
        );

    const auto* right =
        dynamic_cast<const compiler::BoolLiteralExpr*>(
            binary->right.get()
        );

    ASSERT_NE(left, nullptr);
    EXPECT_EQ(left->value, true);

    ASSERT_NE(right, nullptr);
    EXPECT_EQ(right->value, false);
}

TEST(ParserTest, LogicalOr) {
    auto result = parse_test_expression("true || false");

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr = std::get<std::unique_ptr<compiler::Expr>>(result);

    const auto* binary =
        dynamic_cast<const compiler::BinaryExpr*>(expr.get());

    ASSERT_NE(binary, nullptr);
    EXPECT_EQ(binary->op, compiler::BinaryOp::LogicalOr);
    EXPECT_EQ(binary->span.begin, 0);
    EXPECT_EQ(binary->span.end, 13);

    const auto* left =
        dynamic_cast<const compiler::BoolLiteralExpr*>(
            binary->left.get()
        );

    const auto* right =
        dynamic_cast<const compiler::BoolLiteralExpr*>(
            binary->right.get()
        );

    ASSERT_NE(left, nullptr);
    EXPECT_EQ(left->value, true);

    ASSERT_NE(right, nullptr);
    EXPECT_EQ(right->value, false);
}

TEST(ParserTest, MultiplicativeLeftAssociativity) {
    auto result = parse_test_expression("8 / 4 / 2");
    
    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr = std::get<std::unique_ptr<compiler::Expr>>(result);

    const auto* binary = 
        dynamic_cast<const compiler::BinaryExpr*>(expr.get());

    ASSERT_NE(binary, nullptr);
    EXPECT_EQ(binary->op, compiler::BinaryOp::Divide);
    EXPECT_EQ(binary->span.begin, 0);
    EXPECT_EQ(binary->span.end, 9);

    const auto* inner =
        dynamic_cast<const compiler::BinaryExpr*>(
            binary->left.get()
        );

    const auto* right =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            binary->right.get()
        );

    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->op, compiler::BinaryOp::Divide);
    EXPECT_EQ(inner->span.begin, 0);
    EXPECT_EQ(inner->span.end, 5);

    ASSERT_NE(right, nullptr);
    EXPECT_EQ(right->value, 2);
    EXPECT_EQ(right->span.begin, 8);
    EXPECT_EQ(right->span.end, 9);

    const auto* inner_left =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            inner->left.get()
        );

    const auto* inner_right =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            inner->right.get()
        );
    
    ASSERT_NE(inner_left, nullptr);
    EXPECT_EQ(inner_left->value, 8);
    EXPECT_EQ(inner_left->span.begin, 0);
    EXPECT_EQ(inner_left->span.end, 1);

    ASSERT_NE(inner_right, nullptr);
    EXPECT_EQ(inner_right->value, 4);
    EXPECT_EQ(inner_right->span.begin, 4);
    EXPECT_EQ(inner_right->span.end, 5);
}

TEST(ParserTest, AdditiveLeftAssociativity) {
    auto result = parse_test_expression("10 - 3 - 2");

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr = std::get<std::unique_ptr<compiler::Expr>>(result);

    const auto* binary = 
        dynamic_cast<const compiler::BinaryExpr*>(expr.get());

    ASSERT_NE(binary, nullptr);
    EXPECT_EQ(binary->op, compiler::BinaryOp::Subtract);
    EXPECT_EQ(binary->span.begin, 0);
    EXPECT_EQ(binary->span.end, 10);

    const auto* inner =
        dynamic_cast<const compiler::BinaryExpr*>(
            binary->left.get()
        );

    const auto* right =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            binary->right.get()
        );

    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->op, compiler::BinaryOp::Subtract);
    EXPECT_EQ(inner->span.begin, 0);
    EXPECT_EQ(inner->span.end, 6);

    ASSERT_NE(right, nullptr);
    EXPECT_EQ(right->value, 2);
    EXPECT_EQ(right->span.begin, 9);
    EXPECT_EQ(right->span.end, 10);

    const auto* inner_left =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            inner->left.get()
        );

    const auto* inner_right =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            inner->right.get()
        );
    
    ASSERT_NE(inner_left, nullptr);
    EXPECT_EQ(inner_left->value, 10);
    EXPECT_EQ(inner_left->span.begin, 0);
    EXPECT_EQ(inner_left->span.end, 2);

    ASSERT_NE(inner_right, nullptr);
    EXPECT_EQ(inner_right->value, 3);
    EXPECT_EQ(inner_right->span.begin, 5);
    EXPECT_EQ(inner_right->span.end, 6);
}

TEST(ParserTest, MultiplicationPrecedesAddition) {
    auto result = parse_test_expression("1 + 2 * 3");

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr =
        std::get<std::unique_ptr<compiler::Expr>>(result);

    const auto* binary =
        dynamic_cast<const compiler::BinaryExpr*>(expr.get());

    ASSERT_NE(binary, nullptr);
    EXPECT_EQ(binary->op, compiler::BinaryOp::Add);
    EXPECT_EQ(binary->span.begin, 0);
    EXPECT_EQ(binary->span.end, 9);

    const auto* left =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            binary->left.get()
        );

    const auto* inner =
        dynamic_cast<const compiler::BinaryExpr*>(
            binary->right.get()
        );

    ASSERT_NE(left, nullptr);
    EXPECT_EQ(left->value, 1);
    EXPECT_EQ(left->span.begin, 0);
    EXPECT_EQ(left->span.end, 1);

    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->op, compiler::BinaryOp::Multiply);
    EXPECT_EQ(inner->span.begin, 4);
    EXPECT_EQ(inner->span.end, 9);

    const auto* inner_left =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            inner->left.get()
        );

    const auto* inner_right =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            inner->right.get()
        );

    ASSERT_NE(inner_left, nullptr);
    EXPECT_EQ(inner_left->value, 2);
    EXPECT_EQ(inner_left->span.begin, 4);
    EXPECT_EQ(inner_left->span.end, 5);

    ASSERT_NE(inner_right, nullptr);
    EXPECT_EQ(inner_right->value, 3);
    EXPECT_EQ(inner_right->span.begin, 8);
    EXPECT_EQ(inner_right->span.end, 9);
}

TEST(ParserTest, ParenthesesOverridePrecedence) {
    auto result = parse_test_expression("(1 + 2) * 3");

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr = std::get<std::unique_ptr<compiler::Expr>>(result);

    const auto* binary = 
        dynamic_cast<const compiler::BinaryExpr*>(expr.get());

    ASSERT_NE(binary, nullptr);
    EXPECT_EQ(binary->op, compiler::BinaryOp::Multiply);
    EXPECT_EQ(binary->span.begin, 1);
    EXPECT_EQ(binary->span.end, 11);

    const auto* inner =
        dynamic_cast<const compiler::BinaryExpr*>(
            binary->left.get()
        );

    const auto* right =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            binary->right.get()
        );

    ASSERT_NE(inner, nullptr);
    EXPECT_EQ(inner->op, compiler::BinaryOp::Add);
    EXPECT_EQ(inner->span.begin, 1);
    EXPECT_EQ(inner->span.end, 6);

    ASSERT_NE(right, nullptr);
    EXPECT_EQ(right->value, 3);
    EXPECT_EQ(right->span.begin, 10);
    EXPECT_EQ(right->span.end, 11);

    const auto* inner_left =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            inner->left.get()
        );

    const auto* inner_right =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            inner->right.get()
        );
    
    ASSERT_NE(inner_left, nullptr);
    EXPECT_EQ(inner_left->value, 1);
    EXPECT_EQ(inner_left->span.begin, 1);
    EXPECT_EQ(inner_left->span.end, 2);

    ASSERT_NE(inner_right, nullptr);
    EXPECT_EQ(inner_right->value, 2);
    EXPECT_EQ(inner_right->span.begin, 5);
    EXPECT_EQ(inner_right->span.end, 6);
}

TEST(ParserTest, LogicalAndPrecedesLogicalOr) {
    auto result = parse_test_expression("true || false && false");

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr =
        std::get<std::unique_ptr<compiler::Expr>>(result);

    const auto* logical_or =
        dynamic_cast<const compiler::BinaryExpr*>(expr.get());

    ASSERT_NE(logical_or, nullptr);
    EXPECT_EQ(logical_or->op, compiler::BinaryOp::LogicalOr);
    EXPECT_EQ(logical_or->span.begin, 0);
    EXPECT_EQ(logical_or->span.end, 22);

    const auto* left =
        dynamic_cast<const compiler::BoolLiteralExpr*>(
            logical_or->left.get()
        );

    const auto* logical_and =
        dynamic_cast<const compiler::BinaryExpr*>(
            logical_or->right.get()
        );

    ASSERT_NE(left, nullptr);
    EXPECT_EQ(left->value, true);
    EXPECT_EQ(left->span.begin, 0);
    EXPECT_EQ(left->span.end, 4);

    ASSERT_NE(logical_and, nullptr);
    EXPECT_EQ(logical_and->op, compiler::BinaryOp::LogicalAnd);
    EXPECT_EQ(logical_and->span.begin, 8);
    EXPECT_EQ(logical_and->span.end, 22);

    const auto* and_left =
        dynamic_cast<const compiler::BoolLiteralExpr*>(
            logical_and->left.get()
        );

    const auto* and_right =
        dynamic_cast<const compiler::BoolLiteralExpr*>(
            logical_and->right.get()
        );

    ASSERT_NE(and_left, nullptr);
    EXPECT_EQ(and_left->value, false);
    EXPECT_EQ(and_left->span.begin, 8);
    EXPECT_EQ(and_left->span.end, 13);

    ASSERT_NE(and_right, nullptr);
    EXPECT_EQ(and_right->value, false);
    EXPECT_EQ(and_right->span.begin, 17);
    EXPECT_EQ(and_right->span.end, 22);
}

TEST(ParserTest, MixedOperatorPrecedence) {
    auto result =
        parse_test_expression("1 + 2 * 3 < 10 == true || false");

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr =
        std::get<std::unique_ptr<compiler::Expr>>(result);

    const auto* logical_or =
        dynamic_cast<const compiler::BinaryExpr*>(expr.get());

    ASSERT_NE(logical_or, nullptr);
    EXPECT_EQ(logical_or->op, compiler::BinaryOp::LogicalOr);
    EXPECT_EQ(logical_or->span.begin, 0);
    EXPECT_EQ(logical_or->span.end, 31);

    const auto* equality =
        dynamic_cast<const compiler::BinaryExpr*>(
            logical_or->left.get()
        );

    const auto* final_false =
        dynamic_cast<const compiler::BoolLiteralExpr*>(
            logical_or->right.get()
        );

    ASSERT_NE(equality, nullptr);
    EXPECT_EQ(equality->op, compiler::BinaryOp::Equal);
    EXPECT_EQ(equality->span.begin, 0);
    EXPECT_EQ(equality->span.end, 22);

    ASSERT_NE(final_false, nullptr);
    EXPECT_EQ(final_false->value, false);
    EXPECT_EQ(final_false->span.begin, 26);
    EXPECT_EQ(final_false->span.end, 31);

    const auto* comparison =
        dynamic_cast<const compiler::BinaryExpr*>(
            equality->left.get()
        );

    const auto* equality_right =
        dynamic_cast<const compiler::BoolLiteralExpr*>(
            equality->right.get()
        );

    ASSERT_NE(comparison, nullptr);
    EXPECT_EQ(comparison->op, compiler::BinaryOp::Less);
    EXPECT_EQ(comparison->span.begin, 0);
    EXPECT_EQ(comparison->span.end, 14);

    ASSERT_NE(equality_right, nullptr);
    EXPECT_EQ(equality_right->value, true);
    EXPECT_EQ(equality_right->span.begin, 18);
    EXPECT_EQ(equality_right->span.end, 22);

    const auto* additive =
        dynamic_cast<const compiler::BinaryExpr*>(
            comparison->left.get()
        );

    const auto* ten =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            comparison->right.get()
        );

    ASSERT_NE(additive, nullptr);
    EXPECT_EQ(additive->op, compiler::BinaryOp::Add);
    EXPECT_EQ(additive->span.begin, 0);
    EXPECT_EQ(additive->span.end, 9);

    ASSERT_NE(ten, nullptr);
    EXPECT_EQ(ten->value, 10);
    EXPECT_EQ(ten->span.begin, 12);
    EXPECT_EQ(ten->span.end, 14);

    const auto* one =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            additive->left.get()
        );

    const auto* multiply =
        dynamic_cast<const compiler::BinaryExpr*>(
            additive->right.get()
        );

    ASSERT_NE(one, nullptr);
    EXPECT_EQ(one->value, 1);
    EXPECT_EQ(one->span.begin, 0);
    EXPECT_EQ(one->span.end, 1);

    ASSERT_NE(multiply, nullptr);
    EXPECT_EQ(multiply->op, compiler::BinaryOp::Multiply);
    EXPECT_EQ(multiply->span.begin, 4);
    EXPECT_EQ(multiply->span.end, 9);

    const auto* two =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            multiply->left.get()
        );

    const auto* three =
        dynamic_cast<const compiler::IntegerLiteralExpr*>(
            multiply->right.get()
        );

    ASSERT_NE(two, nullptr);
    EXPECT_EQ(two->value, 2);
    EXPECT_EQ(two->span.begin, 4);
    EXPECT_EQ(two->span.end, 5);

    ASSERT_NE(three, nullptr);
    EXPECT_EQ(three->value, 3);
    EXPECT_EQ(three->span.begin, 8);
    EXPECT_EQ(three->span.end, 9);
}

TEST(ParserTest, MissingRightOperand) {
    auto result = parse_test_expression("1 +");

    ASSERT_TRUE(
        std::holds_alternative<compiler::ParseError>(result)
    );

    const auto& error =
        std::get<compiler::ParseError>(result);

    EXPECT_EQ(error.message, "expected expression");
    EXPECT_EQ(error.span.begin, 3);
    EXPECT_EQ(error.span.end, 3);
}

TEST(ParserTest, MissingUnaryOperand) {
    auto result = parse_test_expression("!");

    ASSERT_TRUE(
        std::holds_alternative<compiler::ParseError>(result)
    );

    const auto& error =
        std::get<compiler::ParseError>(result);

    EXPECT_EQ(error.message, "expected expression");
    EXPECT_EQ(error.span.begin, 1);
    EXPECT_EQ(error.span.end, 1);
}

TEST(ParserTest, CallNoArguments) {
    const std::string_view input = "foo()";

    compiler::ExprParseResult result =
        parse_test_expression(input);

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr =
        std::get<std::unique_ptr<compiler::Expr>>(result);

    auto* call =
        dynamic_cast<compiler::CallExpr*>(expr.get());

    ASSERT_NE(call, nullptr);

    EXPECT_EQ(call->callee, "foo");
    EXPECT_TRUE(call->arguments.empty());
    EXPECT_EQ(call->span.begin, 0);
    EXPECT_EQ(call->span.end, 5);
}


TEST(ParserTest, CallMultipleArguments) {
    const std::string_view input =
        "foo(1, 2 + 3, true)";

    compiler::ExprParseResult result =
        parse_test_expression(input);

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr =
        std::get<std::unique_ptr<compiler::Expr>>(result);

    auto* call =
        dynamic_cast<compiler::CallExpr*>(expr.get());

    ASSERT_NE(call, nullptr);

    EXPECT_EQ(call->callee, "foo");
    ASSERT_EQ(call->arguments.size(), 3);

    auto* first =
        dynamic_cast<compiler::IntegerLiteralExpr*>(
            call->arguments[0].get()
        );

    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->value, 1);

    auto* second =
        dynamic_cast<compiler::BinaryExpr*>(
            call->arguments[1].get()
        );

    ASSERT_NE(second, nullptr);
    EXPECT_EQ(second->op, compiler::BinaryOp::Add);

    auto* second_left =
        dynamic_cast<compiler::IntegerLiteralExpr*>(
            second->left.get()
        );

    auto* second_right =
        dynamic_cast<compiler::IntegerLiteralExpr*>(
            second->right.get()
        );

    ASSERT_NE(second_left, nullptr);
    ASSERT_NE(second_right, nullptr);

    EXPECT_EQ(second_left->value, 2);
    EXPECT_EQ(second_right->value, 3);

    auto* third =
        dynamic_cast<compiler::BoolLiteralExpr*>(
            call->arguments[2].get()
        );

    ASSERT_NE(third, nullptr);
    EXPECT_TRUE(third->value);
}


TEST(ParserTest, CallPrecedence) {
    const std::string_view input =
        "foo(1) + 2";

    compiler::ExprParseResult result =
        parse_test_expression(input);

    ASSERT_TRUE(
        std::holds_alternative<std::unique_ptr<compiler::Expr>>(result)
    );

    const auto& expr =
        std::get<std::unique_ptr<compiler::Expr>>(result);

    auto* binary =
        dynamic_cast<compiler::BinaryExpr*>(expr.get());

    ASSERT_NE(binary, nullptr);
    EXPECT_EQ(binary->op, compiler::BinaryOp::Add);

    auto* call =
        dynamic_cast<compiler::CallExpr*>(
            binary->left.get()
        );

    ASSERT_NE(call, nullptr);
    EXPECT_EQ(call->callee, "foo");
    ASSERT_EQ(call->arguments.size(), 1);

    auto* right =
        dynamic_cast<compiler::IntegerLiteralExpr*>(
            binary->right.get()
        );

    ASSERT_NE(right, nullptr);
    EXPECT_EQ(right->value, 2);
}


TEST(ParserTest, CallMissingRightParenthesis) {
    const std::string_view input =
        "foo(1";

    compiler::ExprParseResult result =
        parse_test_expression(input);

    ASSERT_TRUE(
        std::holds_alternative<compiler::ParseError>(result)
    );

    const compiler::ParseError& error =
        std::get<compiler::ParseError>(result);

    EXPECT_EQ(
        error.message,
        "expected ')' after arguments"
    );

    EXPECT_EQ(error.span.begin, input.size());
    EXPECT_EQ(error.span.end, input.size());
}


TEST(ParserTest, VariableDeclarationWithInitializer) {
    const std::string_view input = R"(
fn main() -> void {
    let x: i64 = 42;
}
)";

    compiler::ProgramParseResult result =
        parse_test_program(input);

    ASSERT_TRUE(
        std::holds_alternative<compiler::Program>(result)
    );

    const compiler::Program& program =
        std::get<compiler::Program>(result);

    ASSERT_EQ(program.functions.size(), 1);

    const compiler::FunctionDecl& function =
        program.functions[0];

    EXPECT_EQ(function.name, "main");
    ASSERT_NE(function.body, nullptr);
    ASSERT_EQ(function.body->statements.size(), 1);

    auto* declaration =
        dynamic_cast<compiler::VariableDeclStmt*>(
            function.body->statements[0].get()
        );

    ASSERT_NE(declaration, nullptr);

    EXPECT_EQ(declaration->name, "x");
    EXPECT_EQ(
        declaration->type.kind,
        compiler::TypeKind::I64
    );

    ASSERT_NE(declaration->initializer, nullptr);

    auto* initializer =
        dynamic_cast<compiler::IntegerLiteralExpr*>(
            declaration->initializer.get()
        );

    ASSERT_NE(initializer, nullptr);
    EXPECT_EQ(initializer->value, 42);
}


TEST(ParserTest, VariableDeclarationWithoutInitializer) {
    const std::string_view input = R"(
fn main() -> void {
    let x: i64;
}
)";

    compiler::ProgramParseResult result =
        parse_test_program(input);

    ASSERT_TRUE(
        std::holds_alternative<compiler::Program>(result)
    );

    const compiler::Program& program =
        std::get<compiler::Program>(result);

    ASSERT_EQ(program.functions.size(), 1);

    const compiler::FunctionDecl& function =
        program.functions[0];

    ASSERT_NE(function.body, nullptr);
    ASSERT_EQ(function.body->statements.size(), 1);

    auto* declaration =
        dynamic_cast<compiler::VariableDeclStmt*>(
            function.body->statements[0].get()
        );

    ASSERT_NE(declaration, nullptr);

    EXPECT_EQ(declaration->name, "x");
    EXPECT_EQ(
        declaration->type.kind,
        compiler::TypeKind::I64
    );
    EXPECT_EQ(declaration->initializer, nullptr);
}


TEST(ParserTest, AssignmentStatement) {
    const std::string_view input = R"(
fn main() -> void {
    x = 42;
}
)";

    compiler::ProgramParseResult result =
        parse_test_program(input);

    ASSERT_TRUE(
        std::holds_alternative<compiler::Program>(result)
    );

    const compiler::Program& program =
        std::get<compiler::Program>(result);

    ASSERT_EQ(program.functions.size(), 1);
    ASSERT_EQ(
        program.functions[0].body->statements.size(),
        1
    );

    auto* assignment =
        dynamic_cast<compiler::AssignmentStmt*>(
            program.functions[0]
                .body->statements[0]
                .get()
        );

    ASSERT_NE(assignment, nullptr);

    EXPECT_EQ(assignment->name, "x");

    auto* value =
        dynamic_cast<compiler::IntegerLiteralExpr*>(
            assignment->value.get()
        );

    ASSERT_NE(value, nullptr);
    EXPECT_EQ(value->value, 42);
}


TEST(ParserTest, ReturnWithValue) {
    const std::string_view input = R"(
fn main() -> i64 {
    return 1 + 2;
}
)";

    compiler::ProgramParseResult result =
        parse_test_program(input);

    ASSERT_TRUE(
        std::holds_alternative<compiler::Program>(result)
    );

    const compiler::Program& program =
        std::get<compiler::Program>(result);

    ASSERT_EQ(program.functions.size(), 1);
    ASSERT_EQ(
        program.functions[0].body->statements.size(),
        1
    );

    auto* return_stmt =
        dynamic_cast<compiler::ReturnStmt*>(
            program.functions[0]
                .body->statements[0]
                .get()
        );

    ASSERT_NE(return_stmt, nullptr);
    ASSERT_NE(return_stmt->value, nullptr);

    auto* binary =
        dynamic_cast<compiler::BinaryExpr*>(
            return_stmt->value.get()
        );

    ASSERT_NE(binary, nullptr);
    EXPECT_EQ(binary->op, compiler::BinaryOp::Add);

    auto* left =
        dynamic_cast<compiler::IntegerLiteralExpr*>(
            binary->left.get()
        );

    auto* right =
        dynamic_cast<compiler::IntegerLiteralExpr*>(
            binary->right.get()
        );

    ASSERT_NE(left, nullptr);
    ASSERT_NE(right, nullptr);

    EXPECT_EQ(left->value, 1);
    EXPECT_EQ(right->value, 2);
}


TEST(ParserTest, ReturnWithoutValue) {
    const std::string_view input = R"(
fn main() -> void {
    return;
}
)";

    compiler::ProgramParseResult result =
        parse_test_program(input);

    ASSERT_TRUE(
        std::holds_alternative<compiler::Program>(result)
    );

    const compiler::Program& program =
        std::get<compiler::Program>(result);

    ASSERT_EQ(program.functions.size(), 1);
    ASSERT_EQ(
        program.functions[0].body->statements.size(),
        1
    );

    auto* return_stmt =
        dynamic_cast<compiler::ReturnStmt*>(
            program.functions[0]
                .body->statements[0]
                .get()
        );

    ASSERT_NE(return_stmt, nullptr);
    EXPECT_EQ(return_stmt->value, nullptr);
}


TEST(ParserTest, MultipleStatementsInBlock) {
    const std::string_view input = R"(
fn main() -> i64 {
    let x: i64 = 1;
    x = x + 1;
    return x;
}
)";

    compiler::ProgramParseResult result =
        parse_test_program(input);

    ASSERT_TRUE(
        std::holds_alternative<compiler::Program>(result)
    );

    const compiler::Program& program =
        std::get<compiler::Program>(result);

    ASSERT_EQ(program.functions.size(), 1);

    const auto& statements =
        program.functions[0].body->statements;

    ASSERT_EQ(statements.size(), 3);

    EXPECT_NE(
        dynamic_cast<compiler::VariableDeclStmt*>(
            statements[0].get()
        ),
        nullptr
    );

    EXPECT_NE(
        dynamic_cast<compiler::AssignmentStmt*>(
            statements[1].get()
        ),
        nullptr
    );

    EXPECT_NE(
        dynamic_cast<compiler::ReturnStmt*>(
            statements[2].get()
        ),
        nullptr
    );
}


TEST(ParserTest, IfWithoutElse) {
    const std::string_view input = R"(
fn main() -> void {
    if (true) {
        return;
    }
}
)";

    compiler::ProgramParseResult result =
        parse_test_program(input);

    ASSERT_TRUE(
        std::holds_alternative<compiler::Program>(result)
    );

    const compiler::Program& program =
        std::get<compiler::Program>(result);

    ASSERT_EQ(program.functions.size(), 1);
    ASSERT_EQ(
        program.functions[0].body->statements.size(),
        1
    );

    auto* if_stmt =
        dynamic_cast<compiler::IfStmt*>(
            program.functions[0]
                .body->statements[0]
                .get()
        );

    ASSERT_NE(if_stmt, nullptr);

    auto* condition =
        dynamic_cast<compiler::BoolLiteralExpr*>(
            if_stmt->condition.get()
        );

    ASSERT_NE(condition, nullptr);
    EXPECT_TRUE(condition->value);

    ASSERT_NE(if_stmt->then_branch, nullptr);
    EXPECT_EQ(if_stmt->then_branch->statements.size(), 1);

    EXPECT_NE(
        dynamic_cast<compiler::ReturnStmt*>(
            if_stmt->then_branch
                ->statements[0]
                .get()
        ),
        nullptr
    );

    EXPECT_EQ(if_stmt->else_branch, nullptr);
}


TEST(ParserTest, IfWithElse) {
    const std::string_view input = R"(
fn main() -> void {
    if (true) {
        return;
    } else {
        return;
    }
}
)";

    compiler::ProgramParseResult result =
        parse_test_program(input);

    ASSERT_TRUE(
        std::holds_alternative<compiler::Program>(result)
    );

    const compiler::Program& program =
        std::get<compiler::Program>(result);

    ASSERT_EQ(program.functions.size(), 1);
    ASSERT_EQ(
        program.functions[0].body->statements.size(),
        1
    );

    auto* if_stmt =
        dynamic_cast<compiler::IfStmt*>(
            program.functions[0]
                .body->statements[0]
                .get()
        );

    ASSERT_NE(if_stmt, nullptr);

    auto* condition =
        dynamic_cast<compiler::BoolLiteralExpr*>(
            if_stmt->condition.get()
        );

    ASSERT_NE(condition, nullptr);
    EXPECT_TRUE(condition->value);

    ASSERT_NE(if_stmt->then_branch, nullptr);
    ASSERT_NE(if_stmt->else_branch, nullptr);

    ASSERT_EQ(
        if_stmt->then_branch->statements.size(),
        1
    );

    ASSERT_EQ(
        if_stmt->else_branch->statements.size(),
        1
    );

    EXPECT_NE(
        dynamic_cast<compiler::ReturnStmt*>(
            if_stmt->then_branch
                ->statements[0]
                .get()
        ),
        nullptr
    );

    EXPECT_NE(
        dynamic_cast<compiler::ReturnStmt*>(
            if_stmt->else_branch
                ->statements[0]
                .get()
        ),
        nullptr
    );
}


TEST(ParserTest, WhileStatement) {
    const std::string_view input = R"(
fn main() -> void {
    while (true) {
        return;
    }
}
)";

    compiler::ProgramParseResult result =
        parse_test_program(input);

    ASSERT_TRUE(
        std::holds_alternative<compiler::Program>(result)
    );

    const compiler::Program& program =
        std::get<compiler::Program>(result);

    ASSERT_EQ(program.functions.size(), 1);
    ASSERT_EQ(
        program.functions[0].body->statements.size(),
        1
    );

    auto* while_stmt =
        dynamic_cast<compiler::WhileStmt*>(
            program.functions[0]
                .body->statements[0]
                .get()
        );

    ASSERT_NE(while_stmt, nullptr);

    auto* condition =
        dynamic_cast<compiler::BoolLiteralExpr*>(
            while_stmt->condition.get()
        );

    ASSERT_NE(condition, nullptr);
    EXPECT_TRUE(condition->value);

    ASSERT_NE(while_stmt->body, nullptr);
    ASSERT_EQ(while_stmt->body->statements.size(), 1);

    EXPECT_NE(
        dynamic_cast<compiler::ReturnStmt*>(
            while_stmt->body
                ->statements[0]
                .get()
        ),
        nullptr
    );
}


TEST(ParserTest, FunctionWithParameters) {
    const std::string_view input = R"(
fn add(a: i64, b: i64) -> i64 {
    return a + b;
}
)";

    compiler::ProgramParseResult result =
        parse_test_program(input);

    ASSERT_TRUE(
        std::holds_alternative<compiler::Program>(result)
    );

    const compiler::Program& program =
        std::get<compiler::Program>(result);

    ASSERT_EQ(program.functions.size(), 1);

    const compiler::FunctionDecl& function =
        program.functions[0];

    EXPECT_EQ(function.name, "add");
    ASSERT_EQ(function.parameters.size(), 2);

    EXPECT_EQ(function.parameters[0].name, "a");
    EXPECT_EQ(
        function.parameters[0].type.kind,
        compiler::TypeKind::I64
    );

    EXPECT_EQ(function.parameters[1].name, "b");
    EXPECT_EQ(
        function.parameters[1].type.kind,
        compiler::TypeKind::I64
    );

    EXPECT_EQ(
        function.return_type.kind,
        compiler::TypeKind::I64
    );

    ASSERT_NE(function.body, nullptr);
    ASSERT_EQ(function.body->statements.size(), 1);

    EXPECT_NE(
        dynamic_cast<compiler::ReturnStmt*>(
            function.body->statements[0].get()
        ),
        nullptr
    );
}


TEST(ParserTest, MultipleFunctions) {
    const std::string_view input = R"(
fn add(a: i64, b: i64) -> i64 {
    return a + b;
}

fn main() -> i64 {
    return add(1, 2);
}
)";

    compiler::ProgramParseResult result =
        parse_test_program(input);

    ASSERT_TRUE(
        std::holds_alternative<compiler::Program>(result)
    );

    const compiler::Program& program =
        std::get<compiler::Program>(result);

    ASSERT_EQ(program.functions.size(), 2);

    EXPECT_EQ(program.functions[0].name, "add");
    EXPECT_EQ(program.functions[1].name, "main");

    const compiler::FunctionDecl& main_function =
        program.functions[1];

    ASSERT_NE(main_function.body, nullptr);
    ASSERT_EQ(main_function.body->statements.size(), 1);

    auto* return_stmt =
        dynamic_cast<compiler::ReturnStmt*>(
            main_function.body->statements[0].get()
        );

    ASSERT_NE(return_stmt, nullptr);
    ASSERT_NE(return_stmt->value, nullptr);

    auto* call =
        dynamic_cast<compiler::CallExpr*>(
            return_stmt->value.get()
        );

    ASSERT_NE(call, nullptr);

    EXPECT_EQ(call->callee, "add");
    ASSERT_EQ(call->arguments.size(), 2);

    auto* first =
        dynamic_cast<compiler::IntegerLiteralExpr*>(
            call->arguments[0].get()
        );

    auto* second =
        dynamic_cast<compiler::IntegerLiteralExpr*>(
            call->arguments[1].get()
        );

    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);

    EXPECT_EQ(first->value, 1);
    EXPECT_EQ(second->value, 2);
}


TEST(ParserTest, MissingFunctionArrow) {
    const std::string_view input = R"(
fn main() void {
}
)";

    compiler::ProgramParseResult result =
        parse_test_program(input);

    ASSERT_TRUE(
        std::holds_alternative<compiler::ParseError>(result)
    );

    const compiler::ParseError& error =
        std::get<compiler::ParseError>(result);

    EXPECT_EQ(
        error.message,
        "expected '->' after parameter list"
    );
}


TEST(ParserTest, MissingVariableColon) {
    const std::string_view input = R"(
fn main() -> void {
    let x i64 = 1;
}
)";

    compiler::ProgramParseResult result =
        parse_test_program(input);

    ASSERT_TRUE(
        std::holds_alternative<compiler::ParseError>(result)
    );

    const compiler::ParseError& error =
        std::get<compiler::ParseError>(result);

    EXPECT_EQ(
        error.message,
        "expected ':' after variable name"
    );
}


TEST(ParserTest, MissingReturnSemicolon) {
    const std::string_view input = R"(
fn main() -> i64 {
    return 1
}
)";

    compiler::ProgramParseResult result =
        parse_test_program(input);

    ASSERT_TRUE(
        std::holds_alternative<compiler::ParseError>(result)
    );

    const compiler::ParseError& error =
        std::get<compiler::ParseError>(result);

    EXPECT_EQ(
        error.message,
        "expected ';' after return statement"
    );
}


TEST(ParserTest, MissingBlockRightBrace) {
    const std::string_view input = R"(
fn main() -> void {
    return;
)";

    compiler::ProgramParseResult result =
        parse_test_program(input);

    ASSERT_TRUE(
        std::holds_alternative<compiler::ParseError>(result)
    );

    const compiler::ParseError& error =
        std::get<compiler::ParseError>(result);

    EXPECT_EQ(
        error.message,
        "expected '}' after block"
    );

    EXPECT_EQ(error.span.begin, input.size());
    EXPECT_EQ(error.span.end, input.size());
}


TEST(ParserTest, InvalidTopLevelStatement) {
    const std::string_view input =
        "let x: i64 = 1;";

    compiler::ProgramParseResult result =
        parse_test_program(input);

    ASSERT_TRUE(
        std::holds_alternative<compiler::ParseError>(result)
    );

    const compiler::ParseError& error =
        std::get<compiler::ParseError>(result);

    EXPECT_EQ(
        error.message,
        "expected function declaration"
    );

    EXPECT_EQ(error.span.begin, 0);
    EXPECT_EQ(error.span.end, 3);
}

TEST(ParserTest, ExpressionStatement) {
    const std::string_view input = R"(
fn main() -> void {
    foo();
}
)";

    compiler::ProgramParseResult result =
        parse_test_program(input);

    ASSERT_TRUE(
        std::holds_alternative<compiler::Program>(result)
    );

    const compiler::Program& program =
        std::get<compiler::Program>(result);

    ASSERT_EQ(program.functions.size(), 1);

    const compiler::FunctionDecl& function =
        program.functions[0];

    ASSERT_NE(function.body, nullptr);
    ASSERT_EQ(function.body->statements.size(), 1);

    auto* expr_stmt =
        dynamic_cast<compiler::ExprStmt*>(
            function.body->statements[0].get()
        );

    ASSERT_NE(expr_stmt, nullptr);
    ASSERT_NE(expr_stmt->expression, nullptr);

    auto* call =
        dynamic_cast<compiler::CallExpr*>(
            expr_stmt->expression.get()
        );

    ASSERT_NE(call, nullptr);

    EXPECT_EQ(call->callee, "foo");
    EXPECT_TRUE(call->arguments.empty());
}