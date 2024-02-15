#include "expr.hpp"

Expr::Expr(Token::Loc  loc, const Type  *type)
    : loc{loc}, type{type}
{}

std::int64_t
Expr::getSignedIntValue() const
{
    assert(isConst());
    assert(type->isInteger());
    using T = std::remove_pointer_t<gen::ConstIntVal>;
    auto check = llvm::dyn_cast<T>(loadConstValue());
    assert(check);
    return check->getSExtValue();
}

std::uint64_t
Expr::getUnsignedIntValue() const
{
    assert(isConst());
    assert(type->isInteger());
    using T = std::remove_pointer_t<gen::ConstIntVal>;
    auto check = llvm::dyn_cast<T>(loadConstValue());
    assert(check);
    return check->getZExtValue();
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
