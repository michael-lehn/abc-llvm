#ifndef PARSER_HPP
#define PARSER_HPP

#include "lexer.hpp"
#include "type.hpp"

void semanticError(const char *s);
void semanticError(Token::Loc loc, const char *s);

void parser(void);

// usefull for parsing literal suffix
const Type *parseIntType(void);

#endif // PARSER_HPP

