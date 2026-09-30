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

using ExprParseResult = std::variant<std::unique_ptr<Expr>, ParseError>;
using TokenParseResult = std::variant<Token, ParseError>;

class Parser {
public:
    Parser(
        const SourceFile& source,
        std::vector<Token> tokens
    );

    [[nodiscard]]
    ExprParseResult parse_expression();

private:
    const SourceFile& source_;
    std::vector<Token> tokens_;
    std::size_t current_{0};

    ExprParseResult parse_primary();
    ExprParseResult parse_unary();
    ExprParseResult parse_multiplicative();
    ExprParseResult parse_additive();
    ExprParseResult parse_comparison();
    ExprParseResult parse_equality();
    ExprParseResult parse_logical_and();
    ExprParseResult parse_logical_or();
    
    
    TokenParseResult consume(TokenKind kind, std::string message);

    std::string_view lexeme(const Token& token) const;
    const Token& current() const;
    const Token& previous() const;
    bool at_end() const;
    bool check(TokenKind kind) const;
    const Token& advance();
    bool match(TokenKind kind);
};

}