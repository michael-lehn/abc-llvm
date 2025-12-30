#include <iomanip>
#include <iostream>

#include "gen/cast.hpp"
#include "gen/constant.hpp"
#include "gen/instruction.hpp"
#include "gen/variable.hpp"
#include "lexer/error.hpp"

#include "explicitcast.hpp"

namespace abc {

ExplicitCast::ExplicitCast(ExprPtr &&expr, const Type *toType, lexer::Loc loc)
    : Expr{loc, toType}, expr{std::move(expr)}
{
}

ExprPtr
ExplicitCast::create(ExprPtr &&expr, const Type *toType, lexer::Loc loc)
{
    if (!expr) {
	error::location(loc);
	error::out() << error::setColor(error::BOLD) << loc << ": "
	             << error::setColor(error::BOLD_RED)
	             << "error: " << error::setColor(error::BOLD)
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
	                 << error::setColor(error::BOLD_RED)
	                 << "error: " << error::setColor(error::BOLD)
	                 << "can not cast an expression of "
	                 << "type '" << expr->type << "' to type '" << toType
	                 << "'\n"
	                 << error::setColor(error::NORMAL);
	    error::fatal();
	    return nullptr;
	} else {
	    auto p = new ExplicitCast{std::move(expr), toType, loc};
	    return std::unique_ptr<ExplicitCast>{p};
	}
    }
}

bool
ExplicitCast::hasAddress() const
{
    return true;
}

bool
ExplicitCast::isLValue() const
{
    return false;
}

bool
ExplicitCast::isConst() const
{
    if (expr->type->isArray() && type->isPointer()) {
	return expr->hasConstantAddress();
    } else {
	return expr->isConst();
    }
}

// for code generation
gen::Constant
ExplicitCast::loadConstant() const
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
ExplicitCast::loadValue() const
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
		error::out()
		    << error::setColor(error::BOLD) << loc << ": "
		    << error::setColor(error::BOLD_RED)
		    << "error: " << error::setColor(error::BOLD)
		    << "conversion of out of range value from "
		       " '"
		    << expr->type << "' to '" << type << "' is undefined\n"
		    << error::setColor(error::NORMAL);
		error::fatal();
	    }
	}
    }
    return gen::cast(expr->loadValue(), expr->type, type);
}

gen::Value
ExplicitCast::loadAddress() const
{
    static std::size_t idCount;
    std::stringstream ss;
    ss << ".compound" << idCount++;
    auto tmpId = UStr::create(ss.str()).c_str();
    auto tmpAddr = gen::localVariableDefinition(tmpId, type);
    gen::store(loadValue(), tmpAddr);
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
