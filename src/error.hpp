#ifndef ERROR_HPP
#define ERROR_HPP

#include <ostream>

#include "lexer.hpp"

namespace error {

std::ostream& out(void);
void fatal(void);
void warning(void);

void expected(TokenKind kind);

} // namespace error

#endif // ERROR_HPP
