#include "compiler/parser/parser.hpp"

#include <utility>
#include <cassert>
#include <charconv>

namespace compiler {

using TypeParseResult = std::variant<TypeSyntax, ParseError>;

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

    return parse_call();
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
        advance();

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
        advance();

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

ExprParseResult Parser::parse_call() {
    ExprParseResult primary_result = parse_primary();

    if (std::holds_alternative<ParseError>(primary_result)) {
        return std::get<ParseError>(primary_result);
    }

    ExprPtr primary =
        std::move(std::get<ExprPtr>(primary_result));

    if (!check(TokenKind::LeftParen)) {
        return primary;
    }

    VariableExpr* variable =
        dynamic_cast<VariableExpr*>(primary.get());

    if (variable == nullptr) {
        return ParseError{
            current().span,
            "expected function name before '('"
        };
    }

    SourceOffset begin = primary->span.begin;
    std::string callee = std::move(variable->name);

    advance(); // consume '('

    std::vector<ExprPtr> arguments;

    if (!check(TokenKind::RightParen)) {
        while (true) {
            ExprParseResult argument_result = parse_expression();

            if (std::holds_alternative<ParseError>(argument_result)) {
                return std::get<ParseError>(argument_result);
            }

            arguments.push_back(
                std::move(std::get<ExprPtr>(argument_result))
            );

            if (!match(TokenKind::Comma)) {
                break;
            }
        }
    }

    TokenParseResult close_result =
        consume(TokenKind::RightParen, "expected ')' after arguments");

    if (std::holds_alternative<ParseError>(close_result)) {
        return std::get<ParseError>(close_result);
    }

    const Token& close_token =
        std::get<Token>(close_result);

    auto expr = std::make_unique<CallExpr>();
    expr->callee = std::move(callee);
    expr->arguments = std::move(arguments);
    expr->span.begin = begin;
    expr->span.end = close_token.span.end;

    return expr;
}

TypeParseResult Parser::parse_type() {
    switch (current().kind) {
        case TokenKind::KwI64: {
            const Token& token = advance();
            return TypeSyntax{TypeKind::I64, token.span};
        }

        case TokenKind::KwBool: {
            const Token& token = advance();
            return TypeSyntax{TypeKind::Bool, token.span};
        }

        case TokenKind::KwVoid: {
            const Token& token = advance();
            return TypeSyntax{TypeKind::Void, token.span};
        }

        default:
            return ParseError{current().span, "expected type"};
    }
}

StmtParseResult Parser::parse_statement() {
    switch (current().kind) {
        case TokenKind::KwLet:
            return parse_variable_decl_statement();

        case TokenKind::KwReturn:
            return parse_return_statement();

        case TokenKind::KwIf:
            return parse_if_statement();

        case TokenKind::KwWhile:
            return parse_while_statement();

        case TokenKind::Identifier:
            if (peek(1).kind == TokenKind::Equal) {
                return parse_assignment_statement();
            }
            return parse_expression_statement();

        default:
            return parse_expression_statement();
    }
}

StmtParseResult Parser::parse_variable_decl_statement() {
    const Token& let_token = advance();

    TokenParseResult name_result =
        consume(TokenKind::Identifier, "expected variable name");

    if (std::holds_alternative<ParseError>(name_result)) {
        return std::get<ParseError>(name_result);
    }

    Token name_token = std::get<Token>(name_result);

    std::string name{lexeme(name_token)};
    SourceSpan name_span = name_token.span;

    TokenParseResult colon_result =
        consume(TokenKind::Colon, "expected ':' after variable name");

    if (std::holds_alternative<ParseError>(colon_result)) {
        return std::get<ParseError>(colon_result);
    }

    TypeParseResult type_result = parse_type();

    if (std::holds_alternative<ParseError>(type_result)) {
        return std::get<ParseError>(type_result);
    }

    TypeSyntax type = std::get<TypeSyntax>(type_result);

    ExprPtr initializer = nullptr;

    if (match(TokenKind::Equal)) {
        ExprParseResult init_result = parse_expression();

        if (std::holds_alternative<ParseError>(init_result)) {
            return std::get<ParseError>(init_result);
        }

        initializer = std::move(std::get<ExprPtr>(init_result));
    }

    TokenParseResult semicolon_result =
        consume(TokenKind::Semicolon, "expected ';' after variable declaration");

    if (std::holds_alternative<ParseError>(semicolon_result)) {
        return std::get<ParseError>(semicolon_result);
    }

    const Token& semicolon_token = std::get<Token>(semicolon_result);

    auto stmt = std::make_unique<VariableDeclStmt>();
    stmt->name = std::move(name);
    stmt->name_span = name_span;
    stmt->span.begin = let_token.span.begin;
    stmt->span.end = semicolon_token.span.end;
    stmt->type = type;
    stmt->initializer = std::move(initializer);

    return StmtPtr{std::move(stmt)};
}



StmtParseResult Parser::parse_assignment_statement(){
    TokenParseResult name_result =
        consume(TokenKind::Identifier, "expected variable name");

    if (std::holds_alternative<ParseError>(name_result)) {
        return std::get<ParseError>(name_result);
    }

    Token name_token = std::get<Token>(name_result);

    std::string name{lexeme(name_token)};

    TokenParseResult assignment_result =
        consume(TokenKind::Equal, "expected '=' after identifier");

    if (std::holds_alternative<ParseError>(assignment_result)) {
        return std::get<ParseError>(assignment_result);
    }
    ExprParseResult value_result = parse_expression();

    if (std::holds_alternative<ParseError>(value_result)) {
        return std::get<ParseError>(value_result);
    }

    ExprPtr value = std::move(std::get<ExprPtr>(value_result));

    TokenParseResult semicolon_result = 
        consume(TokenKind::Semicolon, "expected ';' after assignment");

    if (std::holds_alternative<ParseError>(semicolon_result)) {
        return std::get<ParseError>(semicolon_result);
    }

    Token semicolon_token = std::get<Token>(semicolon_result);

    auto stmt = std::make_unique<AssignmentStmt>();
    stmt->name = std::move(name);
    stmt->name_span = name_token.span;
    stmt->value = std::move(value);
    stmt->span.begin = name_token.span.begin;
    stmt->span.end = semicolon_token.span.end;
    
    return StmtPtr{std::move(stmt)};
}

StmtParseResult Parser::parse_return_statement(){
    const Token& return_token = advance();

    ExprPtr value = nullptr;

    if(!check(TokenKind::Semicolon)){
        ExprParseResult expr_result = parse_expression();

        if (std::holds_alternative<ParseError>(expr_result)) {
            return std::get<ParseError>(expr_result);
        }

        value = std::move(std::get<ExprPtr>(expr_result));
    }
    TokenParseResult semicolon_result = 
        consume(TokenKind::Semicolon, "expected ';' after return statement");

    if (std::holds_alternative<ParseError>(semicolon_result)) {
        return std::get<ParseError>(semicolon_result);
    }

    const Token& semicolon_token = std::get<Token>(semicolon_result);

    auto stmt = std::make_unique<ReturnStmt>();
    stmt->value = std::move(value);
    stmt->span.begin = return_token.span.begin;
    stmt->span.end = semicolon_token.span.end;
    
    return StmtPtr{std::move(stmt)};
}

StmtParseResult Parser::parse_expression_statement() {
    ExprParseResult expr_result = parse_expression();


    if (std::holds_alternative<ParseError>(expr_result)) {
        return std::get<ParseError>(expr_result);
    }

    ExprPtr expr = std::move(std::get<ExprPtr>(expr_result));

    TokenParseResult semicolon_result = 
        consume(TokenKind::Semicolon, "expected ';' after expression");

    if (std::holds_alternative<ParseError>(semicolon_result)) {
        return std::get<ParseError>(semicolon_result);
    }
    const Token& semicolon_token = std::get<Token>(semicolon_result);

    auto stmt = std::make_unique<ExprStmt>();
    stmt->span.begin = expr->span.begin;
    stmt->span.end = semicolon_token.span.end;
    stmt->expression = std::move(expr);

    return StmtPtr{std::move(stmt)};
}

StmtParseResult Parser::parse_if_statement() {
    const Token& if_token = advance();

    TokenParseResult leftparen_result =
        consume(TokenKind::LeftParen, "expected '(' after 'if'");

    if (std::holds_alternative<ParseError>(leftparen_result)) {
        return std::get<ParseError>(leftparen_result);
    }

    ExprParseResult condition_result = parse_expression();

    if (std::holds_alternative<ParseError>(condition_result)) {
        return std::get<ParseError>(condition_result);
    }

    ExprPtr condition =
        std::move(std::get<ExprPtr>(condition_result));

    TokenParseResult rightparen_result =
        consume(TokenKind::RightParen, "expected ')' after if condition");

    if (std::holds_alternative<ParseError>(rightparen_result)) {
        return std::get<ParseError>(rightparen_result);
    }

    BlockParseResult then_result = parse_block();

    if (std::holds_alternative<ParseError>(then_result)) {
        return std::get<ParseError>(then_result);
    }

    std::unique_ptr<BlockStmt> then_branch =
        std::move(std::get<std::unique_ptr<BlockStmt>>(then_result));

    std::unique_ptr<BlockStmt> else_branch = nullptr;

    if (match(TokenKind::KwElse)) {
        BlockParseResult else_result = parse_block();

        if (std::holds_alternative<ParseError>(else_result)) {
            return std::get<ParseError>(else_result);
        }

        else_branch =
            std::move(
                std::get<std::unique_ptr<BlockStmt>>(else_result)
            );
    }

    SourceOffset end =
        else_branch != nullptr
            ? else_branch->span.end
            : then_branch->span.end;

    auto stmt = std::make_unique<IfStmt>();
    stmt->condition = std::move(condition);
    stmt->then_branch = std::move(then_branch);
    stmt->else_branch = std::move(else_branch);
    stmt->span.begin = if_token.span.begin;
    stmt->span.end = end;

    return StmtPtr{std::move(stmt)};
}

StmtParseResult Parser::parse_while_statement() {
    const Token& while_token = advance();

    TokenParseResult leftparen_result =
        consume(TokenKind::LeftParen, "expected '(' after 'while'");

    if (std::holds_alternative<ParseError>(leftparen_result)) {
        return std::get<ParseError>(leftparen_result);
    }

    ExprParseResult condition_result = parse_expression();

    if (std::holds_alternative<ParseError>(condition_result)) {
        return std::get<ParseError>(condition_result);
    }

    ExprPtr condition =
        std::move(std::get<ExprPtr>(condition_result));

    TokenParseResult rightparen_result =
        consume(TokenKind::RightParen, "expected ')' after while condition");

    if (std::holds_alternative<ParseError>(rightparen_result)) {
        return std::get<ParseError>(rightparen_result);
    }

    BlockParseResult body_result = parse_block();

    if (std::holds_alternative<ParseError>(body_result)) {
        return std::get<ParseError>(body_result);
    }

    std::unique_ptr<BlockStmt> body =
        std::move(
            std::get<std::unique_ptr<BlockStmt>>(body_result)
        );

    SourceOffset end = body->span.end;

    auto stmt = std::make_unique<WhileStmt>();
    stmt->condition = std::move(condition);
    stmt->body = std::move(body);
    stmt->span.begin = while_token.span.begin;
    stmt->span.end = end;

    return StmtPtr{std::move(stmt)};
}

FunctionParseResult Parser::parse_function() {
    const Token& fn_token = advance();

    TokenParseResult name_result =
        consume(TokenKind::Identifier, "expected function name");

    if (std::holds_alternative<ParseError>(name_result)) {
        return std::get<ParseError>(name_result);
    }

    const Token& name_token =
        std::get<Token>(name_result);

    std::string name{lexeme(name_token)};

    TokenParseResult open_result =
        consume(TokenKind::LeftParen, "expected '(' after function name");

    if (std::holds_alternative<ParseError>(open_result)) {
        return std::get<ParseError>(open_result);
    }

    std::vector<Parameter> parameters;

    if (!check(TokenKind::RightParen)) {
        while (true) {
            ParameterParseResult parameter_result =
                parse_parameter();

            if (std::holds_alternative<ParseError>(parameter_result)) {
                return std::get<ParseError>(parameter_result);
            }

            parameters.push_back(
                std::move(std::get<Parameter>(parameter_result))
            );

            if (!match(TokenKind::Comma)) {
                break;
            }
        }
    }

    TokenParseResult close_result =
        consume(
            TokenKind::RightParen,
            "expected ')' after parameters"
        );

    if (std::holds_alternative<ParseError>(close_result)) {
        return std::get<ParseError>(close_result);
    }

    TokenParseResult arrow_result =
        consume(
            TokenKind::Arrow,
            "expected '->' after parameter list"
        );

    if (std::holds_alternative<ParseError>(arrow_result)) {
        return std::get<ParseError>(arrow_result);
    }

    TypeParseResult return_type_result =
        parse_type();

    if (std::holds_alternative<ParseError>(return_type_result)) {
        return std::get<ParseError>(return_type_result);
    }

    TypeSyntax return_type =
        std::get<TypeSyntax>(return_type_result);

    BlockParseResult body_result =
        parse_block();

    if (std::holds_alternative<ParseError>(body_result)) {
        return std::get<ParseError>(body_result);
    }

    std::unique_ptr<BlockStmt> body =
        std::move(
            std::get<std::unique_ptr<BlockStmt>>(body_result)
        );

    SourceOffset end = body->span.end;

    FunctionDecl function;
    function.name = std::move(name);
    function.name_span = name_token.span;
    function.parameters = std::move(parameters);
    function.return_type = return_type;
    function.body = std::move(body);
    function.span.begin = fn_token.span.begin;
    function.span.end = end;

    return function;
}

ParameterParseResult Parser::parse_parameter() {
    TokenParseResult name_result =
        consume(TokenKind::Identifier, "expected parameter name");

    if (std::holds_alternative<ParseError>(name_result)) {
        return std::get<ParseError>(name_result);
    }

    const Token& name_token =
        std::get<Token>(name_result);

    std::string name{lexeme(name_token)};

    TokenParseResult colon_result =
        consume(TokenKind::Colon, "expected ':' after parameter name");

    if (std::holds_alternative<ParseError>(colon_result)) {
        return std::get<ParseError>(colon_result);
    }

    TypeParseResult type_result = parse_type();

    if (std::holds_alternative<ParseError>(type_result)) {
        return std::get<ParseError>(type_result);
    }

    TypeSyntax type =
        std::get<TypeSyntax>(type_result);

    Parameter parameter;
    parameter.name = std::move(name);
    parameter.name_span = name_token.span;
    parameter.span.begin = name_token.span.begin;
    parameter.span.end = type.span.end;
    parameter.type = type;

    return parameter;
}

BlockParseResult Parser::parse_block() {
    TokenParseResult leftbrace_result =
        consume(TokenKind::LeftBrace, "expected '{'");

    if (std::holds_alternative<ParseError>(leftbrace_result)) {
        return std::get<ParseError>(leftbrace_result);
    }

    const Token& leftbrace_token =
        std::get<Token>(leftbrace_result);

    std::vector<StmtPtr> statements;

    while (!check(TokenKind::RightBrace) && !at_end()) {
        StmtParseResult statement_result = parse_statement();

        if (std::holds_alternative<ParseError>(statement_result)) {
            return std::get<ParseError>(statement_result);
        }

        StmtPtr statement =
            std::move(std::get<StmtPtr>(statement_result));

        statements.push_back(std::move(statement));
    }

    TokenParseResult rightbrace_result =
        consume(TokenKind::RightBrace, "expected '}' after block");

    if (std::holds_alternative<ParseError>(rightbrace_result)) {
        return std::get<ParseError>(rightbrace_result);
    }

    const Token& rightbrace_token =
        std::get<Token>(rightbrace_result);

    auto block = std::make_unique<BlockStmt>();
    block->statements = std::move(statements);
    block->span.begin = leftbrace_token.span.begin;
    block->span.end = rightbrace_token.span.end;

    return block;
}

ProgramParseResult Parser::parse_program() {
    Program program;

    while (!at_end()) {
        if (!check(TokenKind::KwFn)) {
            return ParseError{
                current().span,
                "expected function declaration"
            };
        }

        FunctionParseResult function_result =
            parse_function();

        if (std::holds_alternative<ParseError>(function_result)) {
            return std::get<ParseError>(function_result);
        }

        program.functions.push_back(
            std::move(
                std::get<FunctionDecl>(function_result)
            )
        );
    }

    return program;
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

const Token& Parser::peek(std::size_t lookahead) const {
    assert(current_ + lookahead < tokens_.size());
    return tokens_[current_ + lookahead];
}

}