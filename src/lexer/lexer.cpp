#include "compiler/lexer/lexer.hpp"


namespace compiler {

Lexer::Lexer(const SourceFile& source)
    : source_(source) {}

bool Lexer::at_end() const{
    return offset_ >= source_.size(); 
}

char Lexer::peek(std::size_t lookahead) const{
    return source_.contents()[offset_ + lookahead];
}

void Lexer::advance(){
    offset_++;
}

void Lexer::skip_trivia() {
    while (!at_end()) {
        if (peek() == ' ' || peek() == '\t' ||peek() == '\n' || peek() == '\r') {
            advance();
        } else if (offset_ + 1 < source_.size() && peek() == '/' && peek(1) == '/') {
            advance();
            advance();
            while (!at_end() && peek() != '\n') {
                advance();
            }
        } else {
            break;
        }
    }
}

LexResult Lexer::next() {
    skip_trivia();

    if (at_end()) {
        return Token{
            TokenKind::Eof,
            SourceSpan{offset_, offset_}
        };
    }

    const SourceOffset start = offset_;
    const char c = peek();

    // Identifier or keyword
    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        c == '_') {

        while (!at_end()) {
            const char current = peek();

            if ((current >= 'a' && current <= 'z') ||
                (current >= 'A' && current <= 'Z') ||
                (current >= '0' && current <= '9') ||
                current == '_') {
                advance();
            } else {
                break;
            }
        }

        const std::string_view text =
            source_.contents().substr(start, offset_ - start);

        TokenKind kind = TokenKind::Identifier;

        if (text == "fn") {
            kind = TokenKind::KwFn;
        } else if (text == "let") {
            kind = TokenKind::KwLet;
        } else if (text == "if") {
            kind = TokenKind::KwIf;
        } else if (text == "else") {
            kind = TokenKind::KwElse;
        } else if (text == "while") {
            kind = TokenKind::KwWhile;
        } else if (text == "return") {
            kind = TokenKind::KwReturn;
        } else if (text == "true") {
            kind = TokenKind::KwTrue;
        } else if (text == "false") {
            kind = TokenKind::KwFalse;
        } else if (text == "i64") {
            kind = TokenKind::KwI64;
        } else if (text == "bool") {
            kind = TokenKind::KwBool;
        } else if (text == "void") {
            kind = TokenKind::KwVoid;
        }

        return Token{
            kind,
            SourceSpan{start, offset_}
        };
    }

    // Integer literal
    if (c >= '0' && c <= '9') {
        while (!at_end()) {
            const char current = peek();

            if (current >= '0' && current <= '9') {
                advance();
            } else {
                break;
            }
        }

        return Token{
            TokenKind::IntegerLiteral,
            SourceSpan{start, offset_}
        };
    }

    TokenKind kind;

    switch (c) {
        case '+':
            kind = TokenKind::Plus;
            advance();
            break;

        case '-':
            if (offset_ + 1 < source_.size() && peek(1) == '>') {
                kind = TokenKind::Arrow;
                advance();
            } else {
                kind = TokenKind::Minus;
            }
            advance();
            break;

        case '*':
            kind = TokenKind::Star;
            advance();
            break;

        case '/':
            kind = TokenKind::Slash;
            advance();
            break;

        case '%':
            kind = TokenKind::Percent;
            advance();
            break;

        case '=':
            if (offset_ + 1 < source_.size() && peek(1) == '=') {
                kind = TokenKind::EqualEqual;
                advance();
            } else {
                kind = TokenKind::Equal;
            }
            advance();
            break;

        case '!':
            if (offset_ + 1 < source_.size() && peek(1) == '=') {
                kind = TokenKind::BangEqual;
                advance();
            } else {
                kind = TokenKind::Bang;
            }
            advance();
            break;

        case '<':
            if (offset_ + 1 < source_.size() && peek(1) == '=') {
                kind = TokenKind::LessEqual;
                advance();
            } else {
                kind = TokenKind::Less;
            }
            advance();
            break;

        case '>':
            if (offset_ + 1 < source_.size() && peek(1) == '=') {
                kind = TokenKind::GreaterEqual;
                advance();
            } else {
                kind = TokenKind::Greater;
            }
            advance();
            break;

        case '&':
            if (offset_ + 1 < source_.size() && peek(1) == '&') {
                kind = TokenKind::AmpAmp;
                advance();
                advance();
                break;
            }

            advance();
            return LexError{
                SourceSpan{start, offset_},
                "Expected '&' after '&'."
            };

        case '|':
            if (offset_ + 1 < source_.size() && peek(1) == '|') {
                kind = TokenKind::PipePipe;
                advance();
                advance();
                break;
            }

            advance();
            return LexError{
                SourceSpan{start, offset_},
                "Expected '|' after '|'."
            };

        case '(':
            kind = TokenKind::LeftParen;
            advance();
            break;

        case ')':
            kind = TokenKind::RightParen;
            advance();
            break;

        case '{':
            kind = TokenKind::LeftBrace;
            advance();
            break;

        case '}':
            kind = TokenKind::RightBrace;
            advance();
            break;

        case ':':
            kind = TokenKind::Colon;
            advance();
            break;

        case ';':
            kind = TokenKind::Semicolon;
            advance();
            break;

        case ',':
            kind = TokenKind::Comma;
            advance();
            break;

        default:
            advance();
            return LexError{
                SourceSpan{start, offset_},
                "Unexpected character."
            };
    }

    return Token{
        kind,
        SourceSpan{start, offset_}
    };
}
}