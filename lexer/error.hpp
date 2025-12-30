#ifndef LEXER_ERROR_HPP
#define LEXER_ERROR_HPP

#include <ostream>

#include "loc.hpp"
#include "token.hpp"

namespace abc {
namespace error {

std::ostream &
out(int indent = 0);
void
fatal();
void
warning();

void
undefinedIdentifier(const lexer::Loc &loc, UStr name);

void
unexpected(lexer::Token locToken, lexer::TokenKind expectedTokenKind);
void
unexpectedAfter(lexer::Token locToken, lexer::TokenKind expectedTokenKind);
void
unexpectedBefore(lexer::Token locToken, lexer::TokenKind expectedTokenKind);

bool
expected(lexer::TokenKind kind);
bool
expected(const std::vector<lexer::TokenKind> &kind);
bool
expectedBeforeToken(lexer::TokenKind kind);
bool
expectedBeforeToken(const std::vector<lexer::TokenKind> &kind);
bool
expectedAfterLastToken(lexer::TokenKind kind);
bool
expectedAfterLastToken(const std::vector<lexer::TokenKind> &kind);

enum Color
{
    NORMAL,
    BOLD, // normal bold
    RED,
    BLUE,
    BOLD_RED,
    BOLD_BLUE,
};

std::string
setColor(Color color);

std::ostream &
location(const lexer::Loc &loc);

} // namespace error
} // namespace abc

#endif // LEXER_ERROR_HPP
