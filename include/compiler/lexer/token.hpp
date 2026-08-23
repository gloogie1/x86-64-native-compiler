#pragma once

#include "compiler/source/source_file.hpp"
#include <string_view>

namespace compiler {

enum class TokenKind {
    Eof,

    Identifier,
    IntegerLiteral,

    KwFn,
    KwLet,
    KwIf,
    KwElse,
    KwWhile,
    KwReturn,
    KwTrue,
    KwFalse,
    KwI64,
    KwBool,
    KwVoid,

    Plus,
    Minus,
    Star,
    Slash,
    Percent,

    EqualEqual,
    BangEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,

    AmpAmp,
    PipePipe,
    Bang,

    Equal,
    Arrow,

    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    Colon,
    Semicolon,
    Comma
};

struct Token {
    TokenKind kind;
    SourceSpan span;
};

[[nodiscard]]
std::string_view token_kind_name(TokenKind kind) noexcept;

}//namespace compiler