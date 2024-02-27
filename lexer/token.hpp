#ifndef TOKEN_HPP
#define TOKEN_HPP

#include "loc.hpp"
#include "tokenkind.hpp"
#include "ustr.hpp"

namespace lexer {

class Token
{
    public:
	Token() = default;
	Token(Token &&) = default;
	Token(Loc loc, TokenKind kind, UStr val)
	    : loc{loc}, kind{kind}, val{val}, processedVal{val}
	{}

	Token(Loc loc, TokenKind kind, UStr val, UStr processedVal)
	    : loc{loc}, kind{kind}, val{val}, processedVal{processedVal}
	{}

	Token &operator=(Token &&) = default;

	Loc	    loc;
	TokenKind   kind = TokenKind::BAD;
	UStr	    val;
	UStr	    processedVal;
};

std::ostream &operator<<(std::ostream &out, const Token &token);

} // namespace lexer

#endif // TOKEN_HPP
