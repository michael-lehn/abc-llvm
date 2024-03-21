#include <iomanip>
#include <iostream>

#include "gen/cast.hpp"
#include "gen/constant.hpp"
#include "gen/instruction.hpp"
#include "gen/variable.hpp"
#include "lexer/error.hpp"

#include "explicitcast.hpp"

namespace abc {

ExplicitCast::ExplicitCast(ExprPtr &&expr, const Type *toType,
			   lexer::Loc loc)
    : Expr{loc, toType}, expr{std::move(expr)}
{
}

ExprPtr
ExplicitCast::create(ExprPtr &&expr, const Type *toType, lexer::Loc loc)
{
    if (!expr) {
	error::location(loc);
	error::out() << error::setColor(error::BOLD) << loc
	    << ": " << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << ": expected non-empty expression\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
    }
    assert(expr->type);
    assert(toType);
    if (Type::equals(expr->type, toType)) {
	return expr;
    } else {
	auto loc = expr->loc;
	auto type = Type::explicitCast(expr->type, toType);
	if (!type) {
	    error::location(loc);
	    error::out() << error::setColor(error::BOLD) << loc << ": "
		<< error::setColor(error::BOLD_RED) << "error: "
		<< error::setColor(error::BOLD)
		<< "can not cast an expression of "
		<< "type '" << expr->type << "' to type '" << toType << "'\n"
		<< error::setColor(error::NORMAL);
	    error::fatal();
	    return nullptr;
	}
	auto p = new ExplicitCast{std::move(expr), toType, loc};
	return std::unique_ptr<ExplicitCast>{p};
    }
}

bool
ExplicitCast::hasAddress() const
{
    return false;
}

bool
ExplicitCast::isLValue() const
{
    return false;
}

bool
ExplicitCast::isConst() const
{
    return expr->isConst();
}

// for code generation
gen::Constant
ExplicitCast::loadConstant() const
{
    assert(isConst());
    if (type->isBool()) {
	auto zero = gen::getConstantZero(expr->type);
	return gen::instruction(gen::NE, expr->loadConstant(), zero);
    }
    return gen::cast(expr->loadConstant(), expr->type, type);
}

gen::Value
ExplicitCast::loadValue() const
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
ExplicitCast::loadAddress() const
{
    assert(0 && "Cast expression has no address");
    return nullptr;
}

// for debugging and educational purposes
void
ExplicitCast::print(int indent) const
{
    std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    std::cerr << "cast" << " [ " << type << " ] " << std::endl;
    expr->print(indent + 4);
}

// for printing error messages
void
ExplicitCast::printFlat(std::ostream &out, int prec) const
{
    out << "(";
    out << type;
    out << ")";
    expr->printFlat(out, 14);
}

} // namespace abc
