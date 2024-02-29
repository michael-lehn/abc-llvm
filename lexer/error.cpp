#include <cassert>
#include <cstdlib>
#include <iomanip>
#include <iostream>

#include "error.hpp"
#include "lexer.hpp"

namespace abc { namespace error {

std::ostream &
out(int indent)
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    return std::cerr;
}

void
fatal()
{
    std::exit(1);
}

void
warning()
{
    out() << std::endl << "WARNING" << std::endl << std::endl;
}

bool
expected(lexer::TokenKind kind)
{
    if (lexer::token.kind != kind) {
	out() << lexer::token.loc << ": expected '" << kind << "'"
	    << std::endl;
	fatal();
	return false;
    }
    return true;
}

} } // namespace error, abc
