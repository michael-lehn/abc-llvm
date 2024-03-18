#ifndef LEXER_MACRO_HPP
#define LEXER_MACRO_HPP

#include "util/ustr.hpp"

namespace abc { namespace lexer { namespace macro {

bool ignoreToken();
bool ifndefDirective(const UStr identifier);
void endifDirective();
bool defineDirective(const UStr identifier, const UStr replacement = UStr{});
bool expandMacro(UStr identifier, UStr &replacement);

} } } // namespace macro, lexer, abc

#endif // LEXER_MACRO_HPP
