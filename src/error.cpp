#include <cstdlib>
#include <iostream>

#include "error.hpp"

namespace error {

std::ostream &
out(void)
{
    return std::cerr;
}

void
fatal(void)
{
    std::exit(1);
}

void
expected(TokenKind kind)
{
    if (token.kind != kind) {
	out() << token.loc << ": expected '" << tokenCStr(kind) << "' got '"
	    << token.val.c_str() << "'" << std::endl;
	fatal();
    }
}

} // namespace error
