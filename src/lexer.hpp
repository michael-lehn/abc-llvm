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
    CHARACTER_LITERAL,

    // keywords
    CONST,
    FN,
    RETURN,
    DECL,
    GLOBAL,
    LOCAL,
    STATIC,
    EXTERN,
    FOR,
    WHILE,
    IF,
    ELSE,
THEN,
    ARRAY,
    OF,
    SIZEOF,
    NULLPTR,
    STRUCT,
    UNION,
    TYPE,
    CAST,
    BREAK,
    CONTINUE,
    SWITCH,
    CASE,
    DEFAULT,
    ENUM,

    // punctuators
    DOT,
    DOT3,
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
    HASH,
};

extern struct Token {
    TokenKind kind;
    struct Loc{
	struct Pos {
	    Pos() : line{0}, col{0} {}
	    Pos(std::size_t line, std::size_t col) : line{line}, col{col} {}
	    std::size_t line, col;
	} from, to;
	UStr path;
    } loc;
    UStr val;
    UStr valProcessed; // string literal with escaped characters processed
} token;

bool setLexerInputfile(const char *path);
const char *tokenKindCStr(TokenKind kind);
const char *tokenCStr(TokenKind kind);
TokenKind getToken();
Token::Loc combineLoc(Token::Loc fromLoc, Token::Loc toLoc);

std::ostream &operator<<(std::ostream &out, const Token::Loc &loc);

#endif // LEXER_HPP
