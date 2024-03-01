#include <iomanip>
#include <iostream>

/*
#include "callexpr.hpp"
#include "error.hpp"
#include "gen.hpp"
#include "promotion.hpp"

CallExpr::CallExpr(ExprPtr &&fn, std::vector<ExprPtr> &&param, const Type *type,
		   Token::Loc loc)
    : Expr{loc, type}, fn{std::move(fn)}, param{std::move(param)}
{
}

ExprPtr
CallExpr::create(ExprPtr &&fn, std::vector<ExprPtr> &&param, Token::Loc loc)
{
    auto promotion = promotion::call(std::move(fn), std::move(param), &loc);
    auto p = new CallExpr{std::move(std::get<0>(promotion)),
			  std::move(std::get<1>(promotion)),
			  std::get<2>(promotion),
			  loc};
    return std::unique_ptr<CallExpr>{p};
}

bool
CallExpr::hasAddr() const
{
    return false;
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
gen::ConstVal
CallExpr::loadConstValue() const
{
    assert(isConst());
    return nullptr;
}

gen::Reg
CallExpr::loadValue() const
{
    gen::Reg fnReg = fn->type->isPointer() ? fn->loadValue() : fn->loadAddr();
    auto fnType = fn->type->isPointer() ? fn->type->getRefType() : fn->type;
    std::vector<gen::Reg> fnParam;
    for (const auto &p : param) {
	fnParam.push_back(p->loadValue());
    }
    return gen::call(fnReg, fnType, fnParam);
}

gen::Reg
CallExpr::loadAddr() const
{
    assert(0 && "CallExpr has no address");
    return nullptr;
}

void
CallExpr::condJmp(gen::Label trueLabel, gen::Label falseLabel) const
{
    auto zero = gen::loadZero(type);
    auto cond = gen::cond(gen::NE, loadValue(), zero);
    gen::jmp(cond, trueLabel, falseLabel);
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
    for (const auto &e : param) {
	e->print(indent + 4);
    }
}

void
CallExpr::printFlat(std::ostream &out, int prec) const
{
    fn->printFlat(out, prec);
    out << "(";
    for (std::size_t i = 0; i < param.size(); ++i) {
	out << param[i];
	if (i + 1 < param.size()) {
	    out << ", ";
	}
    }
    out << ")";
}

*/
