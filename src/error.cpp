#include <cstdlib>
#include <iomanip>
#include <iostream>

#include "error.hpp"

namespace error {

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
expected(TokenKind kind)
{
    if (token.kind != kind) {
	out() << token.loc << ": expected '" << tokenCStr(kind) << "' got '"
	    << token.val.c_str() << "'" << std::endl;
	fatal();
	return false;
    }
    return true;
}

} // namespace error
