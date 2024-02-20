#include "expr.hpp"

Expr::Expr(Token::Loc  loc, const Type  *type)
    : loc{loc}, type{type}
{}

gen::ConstIntVal
Expr::getConstIntValue() const
{
    assert(isConst());
    assert(type->isInteger());
    using T = std::remove_pointer_t<gen::ConstIntVal>;
    auto check = llvm::dyn_cast<T>(loadConstValue());
    assert(check);
    return check;
}

std::int64_t
Expr::getSignedIntValue() const
{
    return getConstIntValue()->getSExtValue();
}

std::uint64_t
Expr::getUnsignedIntValue() const
{
    return getConstIntValue()->getZExtValue();
}

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
