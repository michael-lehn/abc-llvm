#ifndef PARSER_PARSETYPE_HPP
#define PARSER_PARSETYPE_HPP

#include "ast/typenode.hpp"

namespace abc {
    
TypeNodePtr parseType_(bool allowZeroDim);

} // namespace abc

#endif // PARSER_PARSETYPE_HPP
