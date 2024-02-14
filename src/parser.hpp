#ifndef PARSER_HPP
#define PARSER_HPP

#include "ast.hpp"
#include "initializerlist.hpp"
#include "lexer.hpp"
#include "type.hpp"

void semanticError(const char *s);
void semanticError(Token::Loc loc, const char *s);

AstPtr parser(void);

const Type *parseType(void);
// usefull for parsing literal suffix
bool parseInitializerList(InitializerList &initList);

#endif // PARSER_HPP
