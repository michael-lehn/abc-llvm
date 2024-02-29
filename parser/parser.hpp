#ifndef PARSER_PARSER_HPP
#define PARSER_PARSER_HPP

#include "ast/ast.hpp"

namespace abc {

AstPtr parser(void);
const Type *parseType(void);

} // namespace abc

#endif // PARSER_PARSER_HPP
