#ifndef LEXER_TOKEN_HPP
#define LEXER_TOKEN_HPP

#include "loc.hpp"
#include "tokenkind.hpp"
#include "util/ustr.hpp"

namespace abc { namespace lexer {

class Token
{
    public:
	Token() = default;
	Token(Token &&) = default;
	Token(const Token &) = default;
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

} } // namespace lexer, abc

#endif // LEXER_TOKEN_HPP
