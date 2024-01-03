#include <cstdio>

#include "lexer.hpp"

int
main(void)
{
    while (getToken() != TokenKind::EOI) {
	std::printf("%zu.%zu-%zu.%zu: %s %s\n",
		token.loc.from.line, token.loc.from.col,
		token.loc.to.line, token.loc.to.col,
		tokenCStr(token.kind),
		token.val.c_str());
	if (token.kind == TokenKind::BAD) {
	    break;
	}
    }
}
