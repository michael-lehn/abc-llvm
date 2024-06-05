#ifndef LEXER_ERROR_HPP
#define LEXER_ERROR_HPP

#include <ostream>

#include "lexer.hpp"
#include "loc.hpp"

namespace abc { namespace error {

std::ostream& out(int indent = 0);
void fatal();
void warning();

void undefinedIdentifier(const lexer::Loc &loc, UStr name);

bool expected(lexer::TokenKind kind);
bool expected(const std::vector<lexer::TokenKind> &kind);
bool expectedBeforeToken(lexer::TokenKind kind);
bool expectedBeforeToken(const std::vector<lexer::TokenKind> &kind);
bool expectedAfterLastToken(lexer::TokenKind kind);
bool expectedAfterLastToken(const std::vector<lexer::TokenKind> &kind);

enum Color
{
    NORMAL,
    BOLD, // normal bold
    RED,
    BLUE,
    BOLD_RED,
    BOLD_BLUE,
};

std::string setColor(Color color);

std::ostream& location(const lexer::Loc &loc);

} } // namespace error, abc

#endif // LEXER_ERROR_HPP
