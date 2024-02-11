#include <iomanip>
#include <iostream>

#include "error.hpp"
#include "gen.hpp"
#include "exprvector.hpp"

ExprVector::ExprVector(std::vector<ExprPtr> &&exprVec, const Type *type,
		       Token::Loc loc)
    : Expr{loc, type}, exprVec{std::move(exprVec)}
{
}

ExprPtr
ExprVector::create(std::vector<ExprPtr> &&exprVec, Token::Loc loc)
{
    auto type = exprVec.size() ? exprVec.back()->type : Type::getVoid();
    auto p = new ExprVector{std::move(exprVec), type, loc};
    return std::unique_ptr<ExprVector>{p};
}

bool
ExprVector::hasAddr() const
{
    return false;
}

bool
ExprVector::isLValue() const
{
    return false;
}

bool
ExprVector::isConst() const
{
    for (const auto &e : exprVec) {
	if (!e->isConst()) {
	    return false;
	}
    }
    return true;
}

// for code generation
gen::ConstVal
ExprVector::loadConstValue() const
{
    return exprVec.back()->loadConstValue();
}

gen::Reg
ExprVector::loadValue() const
{
    gen::Reg reg = nullptr;
    for (const auto &e : exprVec) {
	reg = e->loadValue();
    }
    assert(reg);
    return reg;
}

gen::Reg
ExprVector::loadAddr() const
{
    assert(0 && "ExprVector has no address");
    return nullptr;
}

void
ExprVector::condJmp(gen::Label trueLabel, gen::Label falseLabel) const
{
    auto zero = gen::loadZero(type);
    auto cond = gen::cond(gen::NE, loadValue(), zero);
    gen::jmp(cond, trueLabel, falseLabel);
}

// for debugging and educational purposes
void
ExprVector::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    std::cerr << "expr vector [ " << type << " ] " << std::endl;
    for (const auto &e : exprVec) {
	e->print(indent + 4);
    }
}
