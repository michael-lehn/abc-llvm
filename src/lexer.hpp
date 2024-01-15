#ifndef LEXER_HPP
#define LEXER_HPP

#include <cstddef>
#include <ostream>

#include "ustr.hpp"

enum class TokenKind {
    BAD,
    EOI,
    IDENTIFIER,

    // literals
    DECIMAL_LITERAL,
    HEXADECIMAL_LITERAL,
    OCTAL_LITERAL,
    STRING_LITERAL,

    // keywords
    I8,
    I16,
    I32,
    I64,
    U8,
    U16,
    U32,
    U64,
    FN,
    RETURN,
    DECL,
    GLOBAL,
    LOCAL,
    STATIC,
    FOR,
    WHILE,
    IF,
    ELSE,
    ARRAY,
    OF,
    SIZEOF,

    // punctuators
    SEMICOLON,
    COLON,
    COMMA,
    LBRACE,
    RBRACE,
    LPAREN,
    RPAREN,
    LBRACKET,
    RBRACKET,
    CARET,
    PLUS,
    PLUS2,
    PLUS_EQUAL,
    MINUS,
    MINUS2,
    MINUS_EQUAL,
    ARROW,
    ASTERISK,
    ASTERISK_EQUAL,
    SLASH,
    SLASH_EQUAL,
    PERCENT,
    PERCENT_EQUAL,
    EQUAL,
    EQUAL2,
    NOT,
    NOT_EQUAL,
    GREATER,
    GREATER_EQUAL,
    LESS,
    LESS_EQUAL,
    AND,
    AND2,
    OR,
    OR2,
    QUERY,
};

extern struct Token {
    TokenKind kind;
    struct Loc{
	struct Pos {
	    Pos() : line{0}, col{0} {}
	    Pos(std::size_t line, std::size_t col) : line{line}, col{col} {}
	    std::size_t line, col;
	} from, to;
    } loc;
    UStr val;
    UStr valProcessed; // string literal with escaped characters processed
} token;

bool setLexerInputfile(const char *path);
const char *tokenKindCStr(TokenKind kind);
const char *tokenCStr(TokenKind kind);
TokenKind getToken(void);

std::ostream &operator<<(std::ostream &out, const Token::Loc &loc);

#endif // LEXER_HPP
