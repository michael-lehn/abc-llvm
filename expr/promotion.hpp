#ifndef PROMOTION_HPP
#define PROMOTION_HPP

#include <tuple>

#include "binaryexpr.hpp"
#include "expr.hpp"
#include "type/type.hpp"
//#include "unaryexpr.hpp"

namespace abc { namespace promotion {

/*
 * Rules for call expressions
 */

using CallResult = std::tuple<ExprPtr, std::vector<ExprPtr>, const Type *>;

CallResult call(ExprPtr &&fn, std::vector<ExprPtr> &&param, lexer::Loc *loc);

/*
 * Rules for binary expressions
 */

using BinaryResult = std::tuple<ExprPtr, ExprPtr, const Type *>;

BinaryResult binary(BinaryExpr::Kind kind, ExprPtr &&left, ExprPtr &&right,
		    lexer::Loc *loc = nullptr);

} } // namespace promotion, abc

#endif // PROMOTION_HPP
