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