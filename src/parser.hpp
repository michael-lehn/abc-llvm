#ifndef PARSER_HPP
#define PARSER_HPP

#include "lexer.hpp"

void expectedError(const char *s);
void expected(TokenKind kind);
void semanticError(const char *s);
void semanticError(Token::Loc loc, const char *s);

void parser(void);

#endif // PARSER_HPP

