#include "gen/constant.hpp"
#include "gen/instruction.hpp"
#include "gen/variable.hpp"
#include "lexer/error.hpp"

#include "expr.hpp"

namespace abc {

Expr::Expr(lexer::Loc loc, const Type *type) : loc{loc}, type{type} {}

bool
Expr::hasConstantAddress() const
{
    return false;
}

gen::Value
Expr::loadValueFrom(gen::Value addr) const
{
    return gen::fetch(addr, type);
}

gen::Constant
Expr::loadConstantAddress() const
{
    assert(hasConstantAddress());
    return nullptr;
}

void
Expr::condition(gen::Label trueLabel, gen::Label falseLabel) const
{
    if (type->isFunction()) {
	error::location(loc);
	error::out() << error::setColor(error::BOLD) << loc << ": "
	             << error::setColor(error::BOLD_RED)
	             << "error: " << error::setColor(error::BOLD)
	             << "use '&' to get the address of a function\n"
	             << error::setColor(error::NORMAL);
	error::fatal();
    }
    if (type->isVoid()) {
	error::location(loc);
	error::out() << error::setColor(error::BOLD) << loc << ": "
	             << error::setColor(error::BOLD_RED)
	             << "error: " << error::setColor(error::BOLD)
	             << "expression of type 'void' has no value to test\n"
	             << error::setColor(error::NORMAL);
	error::fatal();
    }
    if (!type->isScalar()) {
	error::location(loc);
	error::out() << error::setColor(error::BOLD) << loc << ": "
	             << error::setColor(error::BOLD_RED)
	             << "error: " << error::setColor(error::BOLD)
	             << "error: scalar required\n"
	             << error::setColor(error::NORMAL);
	error::fatal();
    }
    auto zero = gen::getConstantZero(type);
    auto cond = gen::instruction(gen::conditionOp(type), loadValue(), zero);
    gen::jumpInstruction(cond, trueLabel, falseLabel);
}

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
