#include "expr.hpp"

Expr::Expr(Token::Loc  loc, const Type  *type)
    : loc{loc}, type{type}
{}
