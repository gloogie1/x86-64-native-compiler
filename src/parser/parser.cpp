#include "compiler/parser/parser.hpp"

#include <utility>
#include <cassert>
#include <charconv>

namespace compiler {

Parser::Parser(const SourceFile& source, std::vector<Token> tokens)
    : source_(source), 
    tokens_(std::move(tokens)){
    assert(!tokens_.empty());
    assert(tokens_.back().kind == TokenKind::Eof);
}

ExprParseResult Parser::parse_expression(){
    return parse_logical_or();
}


ExprParseResult Parser::parse_primary(){
    if(check(TokenKind::IntegerLiteral)){
        const Token& token = advance();

        std::string_view text = lexeme(token);

        std::int64_t value;
        const auto result = 
            std::from_chars(text.data(), text.data() + text.size(), value);

        if(result.ec == std::errc::result_out_of_range){
            return ParseError{
                token.span,
                "integer literal is out of range for i64"
            };
        }    
        
        auto expr = std::make_unique<IntegerLiteralExpr>();
        expr->span = token.span;
        expr->value = value;
        
        return expr;
    }

    if(check(TokenKind::KwTrue)||check(TokenKind::KwFalse)){
        const Token& token = advance();
        
        auto expr = std::make_unique<BoolLiteralExpr>();
        expr->span = token.span;
        expr->value = token.kind == TokenKind::KwTrue;

        return expr;
    }

    if(check(TokenKind::Identifier)){
        const Token& token = advance();

        std::string_view text = lexeme(token);

        auto expr = std::make_unique<VariableExpr>();
        expr->span = token.span;
        expr->name = std::string{text};

        return expr;
    }

    if (check(TokenKind::LeftParen)) {
        advance();

        ExprParseResult inner = parse_expression();

        if (std::holds_alternative<ParseError>(inner)) {
            return std::get<ParseError>(inner);
        }
        TokenParseResult close =
            consume(TokenKind::RightParen, "expected ')' after expression");

        if (std::holds_alternative<ParseError>(close)) {
            return std::get<ParseError>(close);
        }

        return inner;
    }

    return ParseError{current().span, "expected expression"};
}

ExprParseResult Parser::parse_unary() {
    if (check(TokenKind::Minus) || check(TokenKind::Bang)) {
        const Token& op_token = advance();

        UnaryOp op = op_token.kind == TokenKind::Minus
                                    ? UnaryOp::Negate
                                    : UnaryOp::LogicalNot;

        ExprParseResult operand_result = parse_unary();

        if (std::holds_alternative<ParseError>(operand_result)) {
            return std::get<ParseError>(operand_result);
        }

        std::unique_ptr<Expr> operand =
            std::move(
                std::get<std::unique_ptr<Expr>>(operand_result)
            );

        auto expr = std::make_unique<UnaryExpr>();
        expr->op = op;
        expr->span.begin = op_token.span.begin;
        expr->span.end = operand->span.end;
        expr->operand = std::move(operand);

        return expr;
    }

    return parse_primary();
}

ExprParseResult Parser::parse_multiplicative() {
    ExprParseResult left_result = parse_unary();
    if (std::holds_alternative<ParseError>(left_result)) {
        return std::get<ParseError>(left_result);
    }
    std::unique_ptr<Expr> left =
        std::move(
            std::get<std::unique_ptr<Expr>>(left_result)
        );
    
    while (
        check(TokenKind::Star) ||
        check(TokenKind::Slash) ||
        check(TokenKind::Percent)
    ) {
        BinaryOp op;
        // consume operator
        const Token& op_token = advance();
        if (op_token.kind == TokenKind::Star) {
            op = BinaryOp::Multiply;
        } else if (op_token.kind == TokenKind::Slash) {
            op = BinaryOp::Divide;
        } else {
            op = BinaryOp::Remainder;
        }

        // parse right operand
        ExprParseResult right_result = parse_unary();
        // build BinaryExpr
        if (std::holds_alternative<ParseError>(right_result)) {
            return std::get<ParseError>(right_result);
        }
        std::unique_ptr<Expr> right =
            std::move(
                std::get<std::unique_ptr<Expr>>(right_result)
            );

        auto expr = std::make_unique<BinaryExpr>();
        expr->op = op;
        expr->span.begin = left->span.begin;
        expr->span.end = right->span.end;
        expr->left = std::move(left);
        expr->right = std::move(right);
        
        left = std::move(expr);
    }
    return left;
}

ExprParseResult Parser::parse_additive() {
    ExprParseResult left_result = parse_multiplicative();
    if (std::holds_alternative<ParseError>(left_result)) {
        return std::get<ParseError>(left_result);
    }
    std::unique_ptr<Expr> left =
        std::move(
            std::get<std::unique_ptr<Expr>>(left_result)
        );
    
    while (
        check(TokenKind::Plus) ||
        check(TokenKind::Minus)
    ) {
        BinaryOp op;
        // consume operator
        const Token& op_token = advance();
        if (op_token.kind == TokenKind::Plus) {
            op = BinaryOp::Add;
        } else {
            op = BinaryOp::Subtract;
        }

        // parse right operand
        ExprParseResult right_result = parse_multiplicative();
        // build BinaryExpr
        if (std::holds_alternative<ParseError>(right_result)) {
            return std::get<ParseError>(right_result);
        }
        std::unique_ptr<Expr> right =
            std::move(
                std::get<std::unique_ptr<Expr>>(right_result)
            );

        auto expr = std::make_unique<BinaryExpr>();
        expr->op = op;
        expr->span.begin = left->span.begin;
        expr->span.end = right->span.end;
        expr->left = std::move(left);
        expr->right = std::move(right);
        
        left = std::move(expr);
    }
    return left;
}


ExprParseResult Parser::parse_comparison() {
    ExprParseResult left_result = parse_additive();

    if (std::holds_alternative<ParseError>(left_result)) {
        return std::get<ParseError>(left_result);
    }

    std::unique_ptr<Expr> left =
        std::move(
            std::get<std::unique_ptr<Expr>>(left_result)
        );

    while (
        check(TokenKind::Less) ||
        check(TokenKind::LessEqual) ||
        check(TokenKind::Greater) ||
        check(TokenKind::GreaterEqual)
    ) {
        BinaryOp op;

        const Token& op_token = advance();

        if (op_token.kind == TokenKind::Less) {
            op = BinaryOp::Less;
        } else if (op_token.kind == TokenKind::LessEqual) {
            op = BinaryOp::LessEqual;
        } else if (op_token.kind == TokenKind::Greater) {
            op = BinaryOp::Greater;
        } else {
            op = BinaryOp::GreaterEqual;
        }

        ExprParseResult right_result = parse_additive();

        if (std::holds_alternative<ParseError>(right_result)) {
            return std::get<ParseError>(right_result);
        }

        std::unique_ptr<Expr> right =
            std::move(
                std::get<std::unique_ptr<Expr>>(right_result)
            );

        auto expr = std::make_unique<BinaryExpr>();
        expr->op = op;
        expr->span.begin = left->span.begin;
        expr->span.end = right->span.end;
        expr->left = std::move(left);
        expr->right = std::move(right);

        left = std::move(expr);
    }

    return left;
}


ExprParseResult Parser::parse_equality() {
    ExprParseResult left_result = parse_comparison();

    if (std::holds_alternative<ParseError>(left_result)) {
        return std::get<ParseError>(left_result);
    }

    std::unique_ptr<Expr> left =
        std::move(
            std::get<std::unique_ptr<Expr>>(left_result)
        );

    while (
        check(TokenKind::EqualEqual) ||
        check(TokenKind::BangEqual)
    ) {
        BinaryOp op;

        const Token& op_token = advance();

        if (op_token.kind == TokenKind::EqualEqual) {
            op = BinaryOp::Equal;
        } else {
            op = BinaryOp::NotEqual;
        }

        ExprParseResult right_result = parse_comparison();

        if (std::holds_alternative<ParseError>(right_result)) {
            return std::get<ParseError>(right_result);
        }

        std::unique_ptr<Expr> right =
            std::move(
                std::get<std::unique_ptr<Expr>>(right_result)
            );

        auto expr = std::make_unique<BinaryExpr>();
        expr->op = op;
        expr->span.begin = left->span.begin;
        expr->span.end = right->span.end;
        expr->left = std::move(left);
        expr->right = std::move(right);

        left = std::move(expr);
    }

    return left;
}

ExprParseResult Parser::parse_logical_and() {
    ExprParseResult left_result = parse_equality();

    if (std::holds_alternative<ParseError>(left_result)) {
        return std::get<ParseError>(left_result);
    }

    std::unique_ptr<Expr> left =
        std::move(
            std::get<std::unique_ptr<Expr>>(left_result)
        );

    while (check(TokenKind::AmpAmp)) {
        const Token& op_token = advance();

        ExprParseResult right_result = parse_equality();

        if (std::holds_alternative<ParseError>(right_result)) {
            return std::get<ParseError>(right_result);
        }

        std::unique_ptr<Expr> right =
            std::move(
                std::get<std::unique_ptr<Expr>>(right_result)
            );

        auto expr = std::make_unique<BinaryExpr>();
        expr->op = BinaryOp::LogicalAnd;
        expr->span.begin = left->span.begin;
        expr->span.end = right->span.end;
        expr->left = std::move(left);
        expr->right = std::move(right);

        left = std::move(expr);
    }

    return left;
}

ExprParseResult Parser::parse_logical_or() {
    ExprParseResult left_result = parse_logical_and();

    if (std::holds_alternative<ParseError>(left_result)) {
        return std::get<ParseError>(left_result);
    }

    std::unique_ptr<Expr> left =
        std::move(
            std::get<std::unique_ptr<Expr>>(left_result)
        );

    while (check(TokenKind::PipePipe)) {
        const Token& op_token = advance();

        ExprParseResult right_result = parse_logical_and();

        if (std::holds_alternative<ParseError>(right_result)) {
            return std::get<ParseError>(right_result);
        }

        std::unique_ptr<Expr> right =
            std::move(
                std::get<std::unique_ptr<Expr>>(right_result)
            );

        auto expr = std::make_unique<BinaryExpr>();
        expr->op = BinaryOp::LogicalOr;
        expr->span.begin = left->span.begin;
        expr->span.end = right->span.end;
        expr->left = std::move(left);
        expr->right = std::move(right);

        left = std::move(expr);
    }

    return left;
}


std::string_view Parser::lexeme(const Token& token) const {
    return source_.contents().substr(
        token.span.begin,
        token.span.end - token.span.begin
    );
}

const Token& Parser::current() const {
    assert(current_ < tokens_.size());
    return tokens_[current_];
}

const Token& Parser::previous() const {
    assert(current_ > 0);
    assert(current_ - 1 < tokens_.size());
    return tokens_[current_ - 1];
}

bool Parser::at_end() const {
    return current().kind == TokenKind::Eof;
}

bool Parser::check(TokenKind kind) const {
    return current().kind == kind;
}

const Token& Parser::advance() {
    const Token& token = current();

    if (!at_end()) {
        current_++;
    }

    return token;
}

bool Parser::match(TokenKind kind) {
    if (!check(kind)) {
        return false;
    }

    advance();
    return true;
}

TokenParseResult Parser::consume(TokenKind kind, std::string message) {
    if (check(kind)) {
        return advance();
    }

    return ParseError{
        current().span,
        std::move(message)
    };
}


}