#ifndef LEXER_MACRO_HPP
#define LEXER_MACRO_HPP

#include <vector>

#include "token.hpp"
#include "util/ustr.hpp"

namespace abc { namespace lexer { namespace macro {

void init();
bool ignoreToken();
bool ifndefDirective(Token identifier);
void endifDirective();
bool defineDirective(Token identifier, std::vector<Token> &&replacement = {});
bool expandMacro(Token identifier);

bool hasToken();
Token getToken();

} } } // namespace macro, lexer, abc

#endif // LEXER_MACRO_HPP
