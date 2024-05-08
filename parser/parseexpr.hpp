#ifndef PARSER_PARSEEXPR_HPP
#define PARSER_PARSEEXPR_HPP

#include "expr/expr.hpp"
#include "type/type.hpp"

namespace abc {
    
ExprPtr parseCompoundExpression(const Type *type,
				const Type **patchedType = nullptr);
ExprPtr parseExpressionList();
ExprPtr parseAssignmentExpression();

} // namespace abc

#endif // PARSER_PARSEEXPR_HPP
