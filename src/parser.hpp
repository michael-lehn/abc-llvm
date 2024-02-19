#ifndef PARSER_HPP
#define PARSER_HPP

#include "ast.hpp"
#include "initializerlist.hpp"
#include "lexer.hpp"
#include "type.hpp"

void semanticError(const char *s);
void semanticError(Token::Loc loc, const char *s);

AstPtr parser(void);

// usefull for parsing literal suffix
const Type *parseType(void);
AstPtr parseCompoundLiteral(const Type *type);

#endif // PARSER_HPP
