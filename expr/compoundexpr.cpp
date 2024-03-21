#include <iomanip>
#include <iostream>
#include <sstream>

#include "gen/cast.hpp"
#include "gen/constant.hpp"
#include "gen/instruction.hpp"
#include "gen/variable.hpp"
#include "lexer/error.hpp"
#include "type/integertype.hpp"

#include "compoundexpr.hpp"
#include "implicitcast.hpp"

namespace abc {

CompoundExpr::CompoundExpr(std::vector<ExprPtr> &&exprVec, const Type *type,
			   lexer::Loc loc)
    : Expr{loc, type}, exprVec{std::move(exprVec)}
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
	if (i < exprVec.size()) {
	    val[i] = exprVec[i]->loadValue();
	} else {
	    val[i] = gen::getConstantZero(type->aggregateType(i));
	}
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
	const auto memberType = type->memberType();
	for (std::size_t i = 0; i < memberType.size(); ++i) {
	    auto memberAddr = gen::pointerToIndex(type, tmpAddr, i);
	    gen::store(val[i], memberAddr);
	}
    } else {
	assert(0);
    }
}

ExprPtr
CompoundExpr::create(std::vector<ExprPtr> &&exprVec, const Type *type,
		     lexer::Loc loc)
{
    assert(type);
    assert(type->hasSize());

    if (exprVec.size() > type->aggregateSize()) {
	error::location(exprVec[type->aggregateSize()]->loc);
	error::out() << exprVec[type->aggregateSize()]->loc
	    << ": error: excess elements in "
	    << (type->isScalar() ? "scalar"
				 : type->isArray() ? "array"
						   : "struct")
	    << " initializer\n";
	error::fatal();
    }
    for (std::size_t i = 0; i < exprVec.size(); ++i) {
	exprVec[i] = ImplicitCast::create(std::move(exprVec[i]),
					  type->aggregateType(i));
    }
    auto p = new CompoundExpr{std::move(exprVec), type, loc};
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
    for (const auto &expr: exprVec) {
	if (!expr->isConst()) {
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
	if (i < exprVec.size()) {
	    val[i] = exprVec[i]->loadConstant();
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
    for (std::size_t i = 0; i < exprVec.size(); ++i) {
	out << exprVec[i];
	if (i + 1 < exprVec.size()) {
	    out << ", ";
	}
    }
    out << "}";
}

} // namespace abc
