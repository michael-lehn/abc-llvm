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

#include "compoundexpr.hpp"
#include "implicitcast.hpp"

namespace abc {

CompoundExpr::CompoundExpr(std::vector<ExprPtr> &&expr, const Type *type,
			   std::vector<Designator> &&designator,
			   std::vector<const Expr *> &&parsedExpr,
			   lexer::Loc loc)
    : Expr{loc, type}, designator{std::move(designator)}
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
    std::vector<const Type *> valType{type->aggregateSize()};
    for (std::size_t i = 0; i < type->aggregateSize(); ++i) {
	val[i] = loadValue(i);
	valType[i] = type->aggregateType(i);
    }

    if (type->isScalar()) {
	gen::store(val[0], tmpAddr, valType[0]->hasVolatileFlag());
    } else if (type->isArray()) {
	for (std::size_t i = 0; i < type->dim(); ++i) {
	    auto index = gen::getConstantInt(i, IntegerType::createSizeType());
	    auto elementAddr = gen::pointerIncrement(type->refType(),
						     tmpAddr,
						     index);
	    gen::store(val[i], elementAddr, valType[i]->hasVolatileFlag());
	}
    } else if (type->isStruct()) {
	for (std::size_t i = 0; i < type->aggregateSize(); ++i) {
	    auto memberAddr = gen::pointerToIndex(type, tmpAddr, i);
	    gen::store(val[i], memberAddr, valType[i]->hasVolatileFlag());
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
    assert(type);
    assert(type->hasSize());
    assert(!designator.size() || designator.size() == parsedExpr.size());

    std::vector<ExprPtr> expr(type->aggregateSize());
    std::vector<const Expr *> parsedExpr_;
    for (std::size_t i = 0, index = 0; i < parsedExpr.size(); ++i, ++index) {
	auto ty = type->aggregateType(i);
	if (designator.size()) {
	    if (std::holds_alternative<lexer::Token>(designator[i])) {
		assert(type->isStruct());
		auto name = std::get<lexer::Token>(designator[i]).val;
		auto index_ = type->memberIndex(name);
		assert(index_.has_value());
		index = index_.value();
		ty = type->memberType(name);
	    } else if (std::holds_alternative<ExprPtr>(designator[i])) {
		assert(type->isArray());
		index = std::get<ExprPtr>(designator[i])->getUnsignedIntValue();
	    }
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
	expr[index] = ImplicitCast::create(std::move(parsedExpr[i]), ty);
	parsedExpr_.push_back(expr[index].get());
    }
    auto p = new CompoundExpr{std::move(expr), type,
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
    return gen::fetch(loadAddress(), type, type->hasVolatileFlag());
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
    std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    printFlat(std::cerr, 0);
}

// for printing error messages
void
CompoundExpr::printFlat(std::ostream &out, int prec) const
{
    if (displayOpt == PAREN) {
	out << "(" << type << ")";
    } else if (displayOpt == BRACE) {
	out << type;
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
