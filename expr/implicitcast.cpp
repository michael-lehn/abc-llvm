#include <iomanip>
#include <iostream>

#include "gen/cast.hpp"
#include "gen/constant.hpp"
#include "gen/instruction.hpp"
#include "gen/variable.hpp"
#include "lexer/error.hpp"

#include "implicitcast.hpp"

namespace abc {

ImplicitCast::ImplicitCast(ExprPtr &&expr, const Type *toType,
			   lexer::Loc loc)
    : Expr{loc, toType}, expr{std::move(expr)}
{
}

ExprPtr
ImplicitCast::create(ExprPtr &&expr, const Type *toType, lexer::Loc loc)
{
    assert(expr->type);
    assert(toType);
    if (Type::equals(expr->type, toType)) {
	return expr;
    } else {
	auto type = Type::convert(expr->type, toType);
	if (!type) {
	    error::out() << loc << ": error: can not convert " << expr->type
		<< " to " << toType << "\n";
	    return nullptr;
	}
	auto p = new ImplicitCast{std::move(expr), type, loc};
	return std::unique_ptr<ImplicitCast>{p};
    }
}

bool
ImplicitCast::hasAddress() const
{
    return false;
}

bool
ImplicitCast::isLValue() const
{
    return false;
}

bool
ImplicitCast::isConst() const
{
    return expr->isConst();
}

// for code generation
gen::Constant
ImplicitCast::loadConstant() const
{
    assert(isConst());
    if (type->isBool()) {
	auto zero = gen::getConstantZero(expr->type);
	return gen::instruction(gen::NE, expr->loadConstant(), zero);
    }
    return gen::cast(expr->loadConstant(), expr->type, type);
}

gen::Value
ImplicitCast::loadValue() const
{
    if (type->isBool()) {
	auto zero = gen::getConstantZero(expr->type);
	return gen::instruction(gen::NE, expr->loadValue(), zero);
    } else if (expr->type->isArray() && type->isPointer()) {
	return expr->loadAddress();
    }
    return gen::cast(expr->loadValue(), expr->type, type);
}

gen::Value
ImplicitCast::loadAddress() const
{
    assert(0 && "Cast expression has no address");
    return nullptr;
}

void
ImplicitCast::condition(gen::Label trueLabel, gen::Label falseLabel) const
{
    auto zero = gen::getConstantZero(type);
    auto cond = gen::instruction(gen::NE, loadValue(), zero);
    gen::jumpInstruction(cond, trueLabel, falseLabel);
}

// for debugging and educational purposes
void
ImplicitCast::print(int indent) const
{
    std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    std::cerr << "cast" << " [ " << type << " ] " << std::endl;
    expr->print(indent + 4);
}

// for printing error messages
void
ImplicitCast::printFlat(std::ostream &out, int prec) const
{
    out << "((";
    out << type;
    out << "))";
    expr->printFlat(out, 14);
}

} // namespace abc