#ifndef LEXER_LEXER_HPP
#define LEXER_LEXER_HPP

#include "loc.hpp"
#include "reader.hpp"
#include "token.hpp"
#include "tokenkind.hpp"

namespace abc { namespace lexer {

extern Token token;

void init();
TokenKind getToken();

} } // namespace lexer, abc

#endif // LEXER_LEXER_HPP
