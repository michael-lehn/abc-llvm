#include <iomanip>
#include <iostream>
#include <sstream>

#include "gen/cast.hpp"
#include "gen/constant.hpp"
#include "gen/instruction.hpp"
#include "gen/variable.hpp"
#include "lexer/error.hpp"
#include "type/integertype.hpp"

#include "exprlist.hpp"
#include "implicitcast.hpp"

namespace abc {

ExprList::ExprList(std::vector<ExprPtr> &&exprVec)
    : Expr{exprVec.front()->loc, exprVec.back()->type}
    , exprVec{std::move(exprVec)}
{
}

ExprPtr
ExprList::create(std::vector<ExprPtr> &&exprVec)
{
    assert(!exprVec.empty());

    auto p = new ExprList{std::move(exprVec)};
    return std::unique_ptr<ExprList>{p};
}

void
ExprList::apply(std::function<bool(const Expr *)> op) const
{
    if (op(this)) {
	for (const auto &expr: exprVec) {
	    expr->apply(op);
	}
    }
}

bool
ExprList::hasAddress() const
{
    return false;
}

bool
ExprList::isLValue() const
{
    return false;
}

bool
ExprList::isConst() const
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
ExprList::loadConstant() const
{
    assert(isConst());
    return exprVec.back()->loadConstant();
}

gen::Value
ExprList::loadValue() const
{
    gen::Value value = nullptr;
    for (const auto &expr: exprVec) {
	value = expr->loadValue();
    }
    return value;
}

gen::Value
ExprList::loadAddress() const
{
    assert(0 && "ExprList has no address");
    return nullptr;
}

// for debugging and educational purposes
void
ExprList::print(int indent) const
{
    std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    printFlat(std::cerr, 0);
}

// for printing error messages
void
ExprList::printFlat(std::ostream &out, int prec) const
{
    for (std::size_t i = 0; i < exprVec.size(); ++i) {
	out << exprVec[i];
	if (i + 1 < exprVec.size()) {
	    out << ", ";
	}
    }
}

} // namespace abc
