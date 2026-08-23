#pragma once

#include "compiler/lexer/token.hpp"
#include "compiler/source/source_file.hpp"

#include <cstddef>
#include <string>
#include <variant>

namespace compiler{

struct LexError {
    SourceSpan span;
    std::string message;
};

using LexResult = std::variant<Token, LexError>;

class Lexer {
public:
    explicit Lexer(const SourceFile& source);

    [[nodiscard]] LexResult next();

private:
    const SourceFile& source_;
    SourceOffset offset_{0};
    [[nodiscard]] bool at_end() const;
    [[nodiscard]] char peek(std::size_t lookahead = 0) const;
    void advance();
    void skip_trivia();
};

}//namespace compiler