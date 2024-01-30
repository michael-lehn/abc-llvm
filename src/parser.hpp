#ifndef PARSER_HPP
#define PARSER_HPP

#include "initializerlist.hpp"
#include "lexer.hpp"
#include "type.hpp"

void semanticError(const char *s);
void semanticError(Token::Loc loc, const char *s);

void parser(void);

const Type *parseType(void);
// usefull for parsing literal suffix
const Type *parseIntType(void);
bool parseInitializerList(InitializerList &initList, bool global);

#endif // PARSER_HPP
