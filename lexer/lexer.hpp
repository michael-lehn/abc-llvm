#ifndef LEXER_LEXER_HPP
#define LEXER_LEXER_HPP

#include "loc.hpp"
#include "reader.hpp"
#include "token.hpp"
#include "tokenkind.hpp"
#include "ustr.hpp"

namespace lexer {

extern Token token;

void init();
TokenKind getToken();

} // namespace lexer

#endif // LEXER_LEXER_HPP
