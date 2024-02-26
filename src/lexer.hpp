#ifndef LEXER_HPP
#define LEXER_HPP

#include <cstddef>
#include <ostream>

#include "tokenkind.hpp"
#include "ustr.hpp"

namespace lexer {

/*

extern struct Token {
    TokenKind kind;
    struct Loc {
	struct Pos {
	    Pos() : line{0}, col{0} {}
	    Pos(std::size_t line, std::size_t col) : line{line}, col{col} {}
	    std::size_t line, col;
	} from, to;
	UStr path;
    } loc;
    UStr val, valRaw;
} token;

bool setLexerInputfile(const char *path);
void addIncludePath(const char *dir);
TokenKind getToken();
Token::Loc combineLoc(Token::Loc fromLoc, Token::Loc toLoc);

std::ostream &operator<<(std::ostream &out, const Token::Loc &loc);
std::string tokenLocStr(const Token::Loc &loc);
*/

} // namespace lexer

#endif // LEXER_HPP
