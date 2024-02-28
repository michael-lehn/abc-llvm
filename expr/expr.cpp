#include "expr.hpp"

namespace abc {

Expr::Expr(lexer::Loc  loc, const Type  *type)
    : loc{loc}, type{type}
{}

gen::ConstantInt
Expr::getConstantInt() const
{
    assert(isConst());
    assert(type->isInteger());
    using T = std::remove_pointer_t<gen::ConstantInt>;
    auto check = llvm::dyn_cast<T>(loadConstant());
    assert(check);
    return check;
}

std::int64_t
Expr::getSignedIntValue() const
{
    return getConstantInt()->getSExtValue();
}

std::uint64_t
Expr::getUnsignedIntValue() const
{
    return getConstantInt()->getZExtValue();
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
    expr->printFlat(out, 1);
    return out;
}

} // namespace abc
