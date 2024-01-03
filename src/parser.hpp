#ifndef PARSER_HPP
#define PARSER_HPP

void expectedError(const char *s);
void expected(TokenKind kind);
void semanticError(const char *s);

void parser(void);

#endif // PARSER_HPP

