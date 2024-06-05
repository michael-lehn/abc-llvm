#include <iomanip>
#include <iostream>

#include "gen/constant.hpp"
#include "gen/function.hpp"
#include "gen/instruction.hpp"
#include "gen/variable.hpp"
#include "lexer/error.hpp"

#include "callexpr.hpp"
#include "promotion.hpp"

namespace abc {

CallExpr::CallExpr(ExprPtr &&fn, std::vector<ExprPtr> &&arg, const Type *type,
		   lexer::Loc loc)
    : Expr{loc, type}, fn{std::move(fn)}, arg{std::move(arg)}
{
    static std::size_t idCount;
    std::stringstream ss;
    ss << ".call" << idCount++;
    tmpId = UStr::create(ss.str());
}

void
CallExpr::initTmp() const
{
    auto tmpAddr = gen::localVariableDefinition(tmpId.c_str(), type);
    gen::store(loadValue(), tmpAddr);
}

ExprPtr
CallExpr::create(ExprPtr &&fn, std::vector<ExprPtr> &&arg, lexer::Loc loc)
{
    assert(fn);
    auto promotion = promotion::call(std::move(fn), std::move(arg), &loc);
    auto p = new CallExpr{std::move(std::get<0>(promotion)),
			  std::move(std::get<1>(promotion)),
			  std::get<2>(promotion),
			  loc};
    return std::unique_ptr<CallExpr>{p};
}

void
CallExpr::apply(std::function<bool(const Expr *)> op) const
{
    if (op(this)) {
	fn->apply(op);
	for (const auto &expr: arg) {
	    expr->apply(op);
	}
    }
}

bool
CallExpr::hasAddress() const
{
    return true;
}

bool
CallExpr::isLValue() const
{
    return false;
}

bool
CallExpr::isConst() const
{
    return false;
}

// for code generation
gen::Constant
CallExpr::loadConstant() const
{
    assert(isConst());
    return nullptr;
}

gen::Value
CallExpr::loadValue() const
{
    std::vector<gen::Value> argValue;
    for (const auto &a : arg) {
	argValue.push_back(a->loadValue());
    }
    auto fnAddr = fn->loadAddress();
    auto call = gen::functionCall(fnAddr, fn->type, argValue);
    return call;
}

gen::Value
CallExpr::loadAddress() const
{
    initTmp();
    return gen::loadAddress(tmpId.c_str());
}

// for debugging and educational purposes
void
CallExpr::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    std::cerr << "call [ " << type << " ] " << std::endl;
    fn->print(indent + 4);
    for (const auto &a : arg) {
	a->print(indent + 4);
    }
}

void
CallExpr::printFlat(std::ostream &out, int prec) const
{
    fn->printFlat(out, prec);
    out << "(";
    for (std::size_t i = 0; i < arg.size(); ++i) {
	out << arg[i];
	if (i + 1 < arg.size()) {
	    out << ", ";
	}
    }
    out << ")";
}

} // namespace abc
