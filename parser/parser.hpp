#ifndef PARSER_PARSER_HPP
#define PARSER_PARSER_HPP

#include "ast/ast.hpp"

namespace abc {

AstPtr parser();
const Type *parseType();

} // namespace abc

#endif // PARSER_PARSER_HPP
