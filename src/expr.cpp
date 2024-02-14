#include "expr.hpp"

Expr::Expr(Token::Loc  loc, const Type  *type)
    : loc{loc}, type{type}
{}

std::ostream &
operator<<(std::ostream &out, const ExprPtr &expr)
{
    expr->printFlat(out, false);
    return out;
}

std::ostream &
operator<<(std::ostream &out, const Expr *expr)
{
    expr->printFlat(out, false);
    return out;
}
