#include <iomanip>
#include <iostream>

#include "gen/cast.hpp"
#include "gen/constant.hpp"
#include "gen/instruction.hpp"
#include "gen/variable.hpp"
#include "lexer/error.hpp"

#include "implicitcast.hpp"

namespace abc {

bool output = true;

ImplicitCast::ImplicitCast(ExprPtr &&expr, const Type *toType,
			   lexer::Loc loc)
    : Expr{loc, toType}, expr{std::move(expr)}
{
}

ExprPtr
ImplicitCast::create(ExprPtr &&expr, const Type *toType)
{
    auto loc = expr->loc;
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
    } else if (toType->isUnboundArray() && Type::convert(expr->type, toType)) {
	return expr;
    } else if (toType->isAuto()
	    || (toType->isArray() && toType->refType()->isAuto())) {
	return expr;
    } else {
	auto loc = expr->loc;
	auto type = Type::convert(expr->type, toType);
	if (!type) {
	    error::location(loc);
	    error::out() << error::setColor(error::BOLD) << loc << ": "
		<< error::setColor(error::BOLD_RED) << "error: "
		<< error::setColor(error::BOLD)
		<< "can not convert an expression of "
		" type '" << expr->type << "' to type '" << toType << "'\n"
		<< error::setColor(error::NORMAL);
	    error::fatal();
	    return nullptr;
	} else {
	    auto p = new ImplicitCast{std::move(expr), type, loc};
	    return std::unique_ptr<ImplicitCast>{p};
	}
    }
}

bool
ImplicitCast::setOutput(bool on)
{
    bool old = output;
    output = on;
    return old;
}

bool
ImplicitCast::hasAddress() const
{
    return true;
}

bool
ImplicitCast::isLValue() const
{
    return false;
}

bool
ImplicitCast::isConst() const
{
    if (expr->type->isArray() && type->isPointer()) {
	return expr->hasConstantAddress();
    } else {
	return expr->isConst();
    }
}

// for code generation
gen::Constant
ImplicitCast::loadConstant() const
{
    assert(isConst());
    if (type->isBool()) {
	auto value = expr->loadConstant();
	if (value->isZeroValue()) {
	    return gen::getFalse();
	} else {
	    return gen::getTrue();
	}
    } else if (expr->type->isArray() && type->isPointer()) {
	return expr->loadConstantAddress();
    } else {
	return gen::cast(expr->loadConstant(), expr->type, type);
    }
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
    if (expr->isConst()) {
	// for constant expression some errors can be caught here ...
	if (expr->type->isFloatType() && type->isUnsignedInteger()) {
	    using T = std::remove_pointer_t<gen::ConstantFloat>;
	    auto val = llvm::dyn_cast<T>(expr->loadConstant());
	    if (val->isNegative()) {
		error::location(loc);
		error::out() << error::setColor(error::BOLD) << loc << ": "
		    << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << "conversion of out of range value from "
		    " '" << expr->type << "' to '" << type
		    << "' is undefined\n"
		    << error::setColor(error::NORMAL);
		error::fatal();
	    }
	}
    }
    return gen::cast(expr->loadValue(), expr->type, type);
}

gen::Value
ImplicitCast::loadAddress() const
{
    static std::size_t idCount;
    std::stringstream ss;
    ss << ".compound" << idCount++;
    auto tmpId = UStr::create(ss.str()).c_str();
    auto tmpAddr = gen::localVariableDefinition(tmpId, type);
    gen::store(loadValue(), tmpAddr);
    return tmpAddr;
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
    if (output) {
	out << "(/*implicit cast*/ ";
	out << type;
	out << ")";
    }
    expr->printFlat(out, 14);
}

} // namespace abc
