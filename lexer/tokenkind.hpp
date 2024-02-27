#ifndef LEXER_TOKENKIND_HPP
#define LEXER_TOKENKIND_HPP

#include <ostream>

namespace lexer {

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
    ASSERT,
    GOTO,
    LABEL,
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
    DO,
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

const char *TokenKindCStr(TokenKind kind);

std::ostream &operator<<(std::ostream &out, TokenKind kind);

} // namespace lexer

#endif // LEXER_TOKENKIND_HPP
