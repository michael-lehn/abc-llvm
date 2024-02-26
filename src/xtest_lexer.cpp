#include <cstdio>

#include "lexer.hpp"

int
main(void)
{
    setLexerInputfile(nullptr);
    while (getToken() != TokenKind::EOI) {
	std::printf("%s:%zu.%zu-%zu.%zu: %s %s\n",
		token.loc.path.c_str(),
		token.loc.from.line, token.loc.from.col,
		token.loc.to.line, token.loc.to.col,
		tokenKindCStr(token.kind),
		token.kind == TokenKind::STRING_LITERAL
		    ? token.valRaw.c_str()
		    : token.val.c_str()
		);
	if (token.kind == TokenKind::BAD) {
	    break;
	}
    }
}
