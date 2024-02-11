#ifndef PROMOTION_HPP
#define PROMOTION_HPP

#include <tuple>

#include "binaryexpr.hpp"
#include "expr.hpp"
#include "type.hpp"
#include "unaryexpr.hpp"

namespace promotion {

/*
 * Rules for call expressions
 */

using CallResult = std::tuple<ExprPtr, std::vector<ExprPtr>, const Type *>;

CallResult call(ExprPtr &&fn, std::vector<ExprPtr> &&param, Token::Loc *loc);

/*
 * Rules for unary expressions
 */

using UnaryResult = std::pair<ExprPtr, const Type *>;

UnaryResult unary(UnaryExpr::Kind kind, ExprPtr &&child,
		  Token::Loc *loc = nullptr);

/*
 * Rules for binary expressions
 */

using BinaryResult = std::tuple<ExprPtr, ExprPtr, const Type *>;

BinaryResult binary(BinaryExpr::Kind kind, ExprPtr &&left, ExprPtr &&right,
		    Token::Loc *loc = nullptr);

/*
 * Rules for conditional expressions
 */

using ConditionalResult = std::tuple<ExprPtr, ExprPtr, ExprPtr, const Type *>;

ConditionalResult conditional(ExprPtr &&cond, ExprPtr &&thenExpr,
			      ExprPtr &&elseExpr, Token::Loc *loc = nullptr);


} // namespace promotion

#endif // PROMOTION_HPP
