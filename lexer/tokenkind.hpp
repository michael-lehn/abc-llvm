#ifndef LEXER_TOKENKIND_HPP
#define LEXER_TOKENKIND_HPP

#include <ostream>

namespace abc { namespace lexer {

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
    FLOAT_DECIMAL_LITERAL,
    FLOAT_HEXADECIMAL_LITERAL,

    // keywords
    ASSERT,
    GOTO,
    LABEL,
    CONST,
    FN,
    RETURN,
    GLOBAL,
    STATIC,
    LOCAL,
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
    BREAK,
    CONTINUE,
    SWITCH,
    CASE,
    DEFAULT,
    ENUM,
    TRUE,
    FALSE,
    VOLATILE,

    // punctuators
    NEWLINE,
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
    CARET_EQUAL,
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
    GREATER2,
    GREATER_EQUAL,
    GREATER2_EQUAL,
    LESS,
    LESS2,
    LESS_EQUAL,
    LESS2_EQUAL,
    AND,
    AND_EQUAL,
    AND2,
    OR,
    OR_EQUAL,
    OR2,
    QUERY,
    HASH,
};

const char *TokenKindCStr(TokenKind kind);

std::ostream &operator<<(std::ostream &out, TokenKind kind);

} } // namespace lexer, abc

#endif // LEXER_TOKENKIND_HPP
