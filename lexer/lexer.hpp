#ifndef LEXER_LEXER_HPP
#define LEXER_LEXER_HPP

#include <filesystem>

#include "loc.hpp"
#include "token.hpp"
#include "tokenkind.hpp"

namespace abc { namespace lexer {

void init();
const std::set<std::filesystem::path> &includedFiles();

extern Token token, lastToken;

TokenKind getToken();


} } // namespace lexer, abc

#endif // LEXER_LEXER_HPP
