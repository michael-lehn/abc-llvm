#ifndef LEXER_ERROR_HPP
#define LEXER_ERROR_HPP

#include <ostream>

#include "lexer.hpp"
#include "loc.hpp"

namespace abc { namespace error {

std::ostream& out(int indent = 0);
void fatal();
void warning();

bool expected(lexer::TokenKind kind);

std::ostream& location(const lexer::Loc &loc);

} } // namespace error, abc

#endif // LEXER_ERROR_HPP
