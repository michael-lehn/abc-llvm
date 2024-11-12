#include <iomanip>
#include <iostream>
#include <sstream>

#include "gen/cast.hpp"
#include "gen/constant.hpp"
#include "gen/instruction.hpp"
#include "gen/variable.hpp"
#include "lexer/error.hpp"
#include "type/arraytype.hpp"
#include "type/integertype.hpp"

#include "badexpr.hpp"
#include "compoundexpr.hpp"
#include "implicitcast.hpp"

namespace abc {

CompoundExpr::CompoundExpr(std::vector<ExprPtr> &&expr, const Type *type,
			   const Type *unpatchedType,
			   std::vector<Designator> &&designator,
			   std::vector<const Expr *> &&parsedExpr,
			   lexer::Loc loc)
    : Expr{loc, type}, unpatchedType{unpatchedType}
    , designator{std::move(designator)}
    , parsedExpr{std::move(parsedExpr)}, expr{std::move(expr)}
{
    static std::size_t idCount;
    std::stringstream ss;
    ss << ".compound" << idCount++;
    tmpId = UStr::create(ss.str());
}

void
CompoundExpr::initTmp() const
{
    auto tmpAddr = gen::localVariableDefinition(tmpId.c_str(), type);

    std::vector<gen::Value> val{type->aggregateSize()};
    for (std::size_t i = 0; i < type->aggregateSize(); ++i) {
	val[i] = loadValue(i);
    }

    if (type->isScalar()) {
	gen::store(val[0], tmpAddr);
    } else if (type->isArray()) {
	for (std::size_t i = 0; i < type->dim(); ++i) {
	    auto index = gen::getConstantInt(i, IntegerType::createSizeType());
	    auto elementAddr = gen::pointerIncrement(type->refType(),
						     tmpAddr,
						     index);
	    gen::store(val[i], elementAddr);
	}
    } else if (type->isStruct()) {
	for (std::size_t i = 0; i < type->aggregateSize(); ++i) {
	    auto memberAddr = gen::pointerToIndex(type, tmpAddr, i);
	    gen::store(val[i], memberAddr);
	}
    } else {
	assert(0);
    }
}

ExprPtr
CompoundExpr::create(std::vector<Designator> &&designator,
		     std::vector<ExprPtr> &&parsedExpr, const Type *type,
		     lexer::Loc loc)
{
    auto unpatchedType = type;
    if (!type) {
	return BadExpr::create(
		    UStr::create("type expected for compound expression"),
		    loc);
    } else if (type->isUnboundArray()) {
	// if parsedExpr is empty it is a zero size array
	type = Type::patchUnboundArray(type, parsedExpr.size());
    }

    if (!type->hasSize()) {
	return BadExpr::create(
		    UStr::create("type with size zero is not supported"),
		    loc);
    }

    std::vector<ExprPtr> expr(type->aggregateSize());
    std::vector<const Expr *> parsedExpr_;
    for (std::size_t i = 0, index = 0; i < parsedExpr.size(); ++i, ++index) {
	if (std::holds_alternative<lexer::Token>(designator[i])) {
	    auto name = std::get<lexer::Token>(designator[i]).val;
	    auto index_ = type->memberIndex(name);
	    if (!index_) {
		error::location(parsedExpr[i]->loc);
		error::out() << error::setColor(error::BOLD)
		    << parsedExpr[i]->loc << ": "
		    << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << "'" << name << "' is not a member "
		    << "of type '" << unpatchedType << "'.\n"
		    << error::setColor(error::NORMAL);
		return BadExpr::create(
			    UStr::create("illegal member"),
			    parsedExpr[i]->loc);
	    }
	    index = index_.value();
	} else if (std::holds_alternative<ExprPtr>(designator[i])) {
	    const auto &d = std::get<ExprPtr>(designator[i]);
	    if (!type->isArray()) {
		error::location(d->loc);
		error::out() << error::setColor(error::BOLD)
		    << d->loc << ": "
		    << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << "desigantor for array index, but "
		    << "'" << unpatchedType << "' is not an array type.\n"
		    << error::setColor(error::NORMAL);
		return BadExpr::create(
			    UStr::create("illegal desigantor"),
			    d->loc);
	    } else if (!d->isConst() || !d->type->isInteger()) {
		error::location(d->loc);
		error::out() << error::setColor(error::BOLD)
		    << loc << ": "
		    << error::setColor(error::BOLD_RED) << "error: "
		    << error::setColor(error::BOLD)
		    << "constant integer expression expected\n"
		    << error::setColor(error::NORMAL);
		error::fatal();
		return BadExpr::create(
			    UStr::create("constant integer expression "
					 "expected"),
			    d->loc);
	    }
	    index = d->getUnsignedIntValue();
	}
	if (index >= type->aggregateSize()) {
	    error::location(parsedExpr[i]->loc);
	    error::out() << error::setColor(error::BOLD)
		<< parsedExpr[i]->loc << ": "
		<< error::setColor(error::BOLD_RED) << "error: "
		<< error::setColor(error::BOLD)
		<< "excess elements in initializer\n"
		<< error::setColor(error::NORMAL);
	    return BadExpr::create(
			UStr::create("excess elements in initializer"),
			parsedExpr[i]->loc);
	}
	if (expr[index]) {
	    error::location(parsedExpr[i]->loc);
	    error::out() << error::setColor(error::BOLD)
		<< parsedExpr[i]->loc << ": "
		<< error::setColor(error::BOLD_BLUE) << "warning: "
		<< error::setColor(error::BOLD)
		<< "initializer overrides prior initialization\n"
		<< error::setColor(error::NORMAL);
	    error::location(expr[index]->loc);
	    error::out() << error::setColor(error::BOLD)
		<< expr[index]->loc << ": "
		<< error::setColor(error::BOLD_BLUE) << "note: "
		<< error::setColor(error::BOLD)
		<< "previous initialization here\n"
		<< error::setColor(error::NORMAL);
	}
	auto ty = type->aggregateType(index);
	expr[index] = ImplicitCast::create(std::move(parsedExpr[i]), ty);
	parsedExpr_.push_back(expr[index].get());
    }
    auto p = new CompoundExpr{std::move(expr), type, unpatchedType,
			      std::move(designator),
			      std::move(parsedExpr_),
			      loc};
    return std::unique_ptr<CompoundExpr>{p};
}

void
CompoundExpr::setDisplayOpt(DisplayOpt opt) const
{
    displayOpt = opt;
}

bool
CompoundExpr::hasAddress() const
{
    return true;
}

bool
CompoundExpr::isLValue() const
{
    return false;
}

bool
CompoundExpr::isConst() const
{
    for (const auto &e: expr) {
	if (e && !e->isConst()) {
	    return false;
	}
    }
    return true;
}

// for code generation
gen::Constant
CompoundExpr::loadConstant() const
{
    assert(isConst());
    assert(type);

    std::vector<gen::Constant> val{type->aggregateSize()};
    for (std::size_t i = 0; i < type->aggregateSize(); ++i) {
	if (expr[i]) {
	    val[i] = expr[i]->loadConstant();
	} else {
	    val[i] = gen::getConstantZero(type->aggregateType(i));
	}
    }
    if (type->isScalar()) {
	return val[0];
    } else if (type->isArray()) {
	return gen::getConstantArray(val, type);
    } else if (type->isStruct()) {
	return gen::getConstantStruct(val, type);
    } else {
	assert(0);
	return nullptr;
    }
}

gen::Value
CompoundExpr::loadValue() const
{
    return gen::fetch(loadAddress(), type);
}

gen::Value
CompoundExpr::loadAddress() const
{
    initTmp();
    return gen::loadAddress(tmpId.c_str());
}

gen::Constant
CompoundExpr::loadConstant(std::size_t index) const
{
    assert(index < type->aggregateSize());
    if (expr[index]) {
	return expr[index]->loadConstant();
    } else {
	return gen::getConstantZero(type->aggregateType(index));
    }
}

gen::Value
CompoundExpr::loadValue(std::size_t index) const
{
    if (expr[index]) {
	return expr[index]->loadValue();
    } else {
	return gen::getConstantZero(type->aggregateType(index));
    }
}

// for debugging and educational purposes
void
CompoundExpr::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    printFlat(std::cerr, 0);
}

// for printing error messages
void
CompoundExpr::printFlat(std::ostream &out, int prec) const
{
    if (displayOpt == PAREN) {
	out << "(" << unpatchedType << ")";
    } else if (displayOpt == BRACE) {
	out << unpatchedType;
    }
    out << "{";
    for (std::size_t i = 0; i < parsedExpr.size(); ++i) {
	if (designator.size()) {
	    if (std::holds_alternative<lexer::Token>(designator[i])) {
		out << "." << std::get<lexer::Token>(designator[i]).val;
		out << " = ";
	    } else if (std::holds_alternative<ExprPtr>(designator[i])) {
		out << "[" << std::get<ExprPtr>(designator[i]) << "]";
		out << " = ";
	    }
	}
	out << parsedExpr[i];
	if (i + 1 < parsedExpr.size()) {
	    out << ", ";
	}
    }
    out << "}";
}

} // namespace abc
