#pragma once

#include "compiler/ast/ast.hpp"
#include "compiler/lexer/token.hpp"
#include "compiler/source/source_file.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace compiler {

struct ParseError {
    SourceSpan span;
    std::string message;
};

using ExprParseResult =
    std::variant<std::unique_ptr<Expr>, ParseError>;

using TokenParseResult =
    std::variant<Token, ParseError>;

using TypeParseResult =
    std::variant<TypeSyntax, ParseError>;

using StmtParseResult =
    std::variant<StmtPtr, ParseError>;

using BlockParseResult =
    std::variant<std::unique_ptr<BlockStmt>, ParseError>;

using ParameterParseResult =
    std::variant<Parameter, ParseError>;

using FunctionParseResult =
    std::variant<FunctionDecl, ParseError>;

using ProgramParseResult =
    std::variant<Program, ParseError>;

class Parser {
public:
    Parser(
        const SourceFile& source,
        std::vector<Token> tokens
    );

    [[nodiscard]]
    ExprParseResult parse_expression();

    [[nodiscard]]
    ProgramParseResult parse_program();

private:
    const SourceFile& source_;
    std::vector<Token> tokens_;
    std::size_t current_{0};

    // Expressions
    ExprParseResult parse_primary();
    ExprParseResult parse_call();
    ExprParseResult parse_unary();
    ExprParseResult parse_multiplicative();
    ExprParseResult parse_additive();
    ExprParseResult parse_comparison();
    ExprParseResult parse_equality();
    ExprParseResult parse_logical_and();
    ExprParseResult parse_logical_or();

    // Types
    TypeParseResult parse_type();

    // Statements
    StmtParseResult parse_statement();
    StmtParseResult parse_variable_decl_statement();
    StmtParseResult parse_assignment_statement();
    StmtParseResult parse_return_statement();
    StmtParseResult parse_expression_statement();
    StmtParseResult parse_if_statement();
    StmtParseResult parse_while_statement();

    BlockParseResult parse_block();

    // Functions
    ParameterParseResult parse_parameter();
    FunctionParseResult parse_function();

    // Helpers
    std::string_view lexeme(const Token& token) const;
    const Token& current() const;
    const Token& previous() const;
    const Token& peek(std::size_t lookahead) const;

    bool at_end() const;
    bool check(TokenKind kind) const;
    const Token& advance();
    bool match(TokenKind kind);

    TokenParseResult consume(
        TokenKind kind,
        std::string message
    );
};

}