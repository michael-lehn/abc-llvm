#ifndef EXPR_PROMOTION_HPP
#define EXPR_PROMOTION_HPP

#include <tuple>

#include "type/type.hpp"

#include "binaryexpr.hpp"
#include "expr.hpp"
#include "unaryexpr.hpp"

namespace abc {
namespace promotion {

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

/*
 * Rules for unary expressions
 */

using UnaryResult = std::pair<ExprPtr, const Type *>;

UnaryResult unary(UnaryExpr::Kind kind, ExprPtr &&child,
                  lexer::Loc *loc = nullptr);

} // namespace promotion
} // namespace abc

#endif // EXPR_PROMOTION_HPP
