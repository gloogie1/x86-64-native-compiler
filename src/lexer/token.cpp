#include "compiler/lexer/token.hpp"

namespace compiler {

std::string_view token_kind_name(TokenKind kind) noexcept {
    switch (kind) {
        case TokenKind::Eof: return "Eof";                         // end of file

        case TokenKind::Identifier: return "Identifier";           // foo, x, my_value
        case TokenKind::IntegerLiteral: return "IntegerLiteral";   // 0, 42, 123

        case TokenKind::KwFn: return "KwFn";                       // fn
        case TokenKind::KwLet: return "KwLet";                     // let
        case TokenKind::KwIf: return "KwIf";                       // if
        case TokenKind::KwElse: return "KwElse";                   // else
        case TokenKind::KwWhile: return "KwWhile";                 // while
        case TokenKind::KwReturn: return "KwReturn";               // return
        case TokenKind::KwTrue: return "KwTrue";                   // true
        case TokenKind::KwFalse: return "KwFalse";                 // false
        case TokenKind::KwI64: return "KwI64";                     // i64
        case TokenKind::KwBool: return "KwBool";                   // bool
        case TokenKind::KwVoid: return "KwVoid";                   // void

        case TokenKind::Plus: return "Plus";                       // +
        case TokenKind::Minus: return "Minus";                     // -
        case TokenKind::Star: return "Star";                       // *
        case TokenKind::Slash: return "Slash";                     // /
        case TokenKind::Percent: return "Percent";                 // %

        case TokenKind::EqualEqual: return "EqualEqual";           // ==
        case TokenKind::BangEqual: return "BangEqual";             // !=
        case TokenKind::Less: return "Less";                       // <
        case TokenKind::LessEqual: return "LessEqual";             // <=
        case TokenKind::Greater: return "Greater";                 // >
        case TokenKind::GreaterEqual: return "GreaterEqual";       // >=

        case TokenKind::AmpAmp: return "AmpAmp";                   // &&
        case TokenKind::PipePipe: return "PipePipe";               // ||
        case TokenKind::Bang: return "Bang";                       // !

        case TokenKind::Equal: return "Equal";                     // =
        case TokenKind::Arrow: return "Arrow";                     // ->

        case TokenKind::LeftParen: return "LeftParen";             // (
        case TokenKind::RightParen: return "RightParen";           // )
        case TokenKind::LeftBrace: return "LeftBrace";             // {
        case TokenKind::RightBrace: return "RightBrace";           // }
        case TokenKind::Colon: return "Colon";                     // :
        case TokenKind::Semicolon: return "Semicolon";             // ;
        case TokenKind::Comma: return "Comma";                     // ,
    }

    return "Unknown";
}

} // namespace compiler