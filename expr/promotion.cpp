#include <iostream>

// #include "callexpr.hpp"
// #include "castexpr.hpp"
#include "lexer/error.hpp"
#include "promotion.hpp"

namespace abc { namespace promotion {

BinaryResult
binary(BinaryExpr::Kind kind, ExprPtr &&left, ExprPtr &&right, lexer::Loc *loc)
{
    return std::make_tuple(std::move(left), std::move(right), left->type);
}

} } // namespace promotion, abc
