#include <iostream>
#include <iomanip>

#include "conditionalexpr.hpp"
#include "gen.hpp"
#include "promotion.hpp"

ConditionalExpr::ConditionalExpr(ExprPtr &&cond, ExprPtr &&thenExpr,
				 ExprPtr &&elseExpr, const Type *type,
				 Token::Loc loc)
    : Expr{loc, type}, cond{std::move(cond)}, thenExpr{std::move(thenExpr)}
    , elseExpr{std::move(elseExpr)}
{
}


ExprPtr
ConditionalExpr::create(ExprPtr &&cond, ExprPtr &&thenExpr, ExprPtr &&elseExpr,
			Token::Loc loc)
{
    auto promotion = promotion::conditional(std::move(cond),
					    std::move(thenExpr),
					    std::move(elseExpr),
					    &loc);
    auto p = new ConditionalExpr{std::move(std::get<0>(promotion)),
				 std::move(std::get<1>(promotion)),
				 std::move(std::get<2>(promotion)),
				 std::get<3>(promotion),
				 loc};
    return std::unique_ptr<ConditionalExpr>{p};
}

bool
ConditionalExpr::hasAddr() const
{
    return thenExpr->hasAddr() && elseExpr->hasAddr();
}

bool
ConditionalExpr::isLValue() const
{
    return thenExpr->isLValue() && elseExpr->isLValue();
}

bool
ConditionalExpr::isConst() const
{
    if (!cond->isConst()) {
	return false;
    }
    auto condVal = cond->loadConstValue();
    return !condVal->isZeroValue()
	? thenExpr->isConst()
	: elseExpr->isConst();
}

// for code generation
gen::ConstVal
ConditionalExpr::loadConstValue() const
{
    assert(isConst());
    auto condVal = cond->loadConstValue();
    return !condVal->isZeroValue()
	? thenExpr->loadConstValue()
	: elseExpr->loadConstValue();
}

gen::Reg
ConditionalExpr::loadValue() const
{
    auto thenLabel = gen::getLabel("condTrue");
    auto elseLabel = gen::getLabel("condFalse");
    auto endLabel = gen::getLabel("end");

    cond->condJmp(thenLabel, elseLabel);
    gen::labelDef(thenLabel);
    auto condTrue = thenExpr->loadValue();
    thenLabel = gen::jmp(endLabel); // update needed for phi
    gen::labelDef(elseLabel);
    auto condFalse = elseExpr->loadValue();
    elseLabel = gen::jmp(endLabel); // update needed for phi
    gen::labelDef(endLabel);
    return gen::phi(condTrue, thenLabel, condFalse, elseLabel, type);
}

gen::Reg
ConditionalExpr::loadAddr() const
{
    assert(hasAddr());

    auto thenLabel = gen::getLabel("condTrue");
    auto elseLabel = gen::getLabel("condFalse");
    auto endLabel = gen::getLabel("end");

    cond->condJmp(thenLabel, elseLabel);
    gen::labelDef(thenLabel);
    auto condTrue = thenExpr->loadAddr();
    thenLabel = gen::jmp(endLabel); // update needed for phi
    gen::labelDef(elseLabel);
    auto condFalse = elseExpr->loadAddr();
    elseLabel = gen::jmp(endLabel); // update needed for phi
    gen::labelDef(endLabel);
    return gen::phi(condTrue, thenLabel, condFalse, elseLabel,
		    Type::getPointer(type));
}

void
ConditionalExpr::condJmp(gen::Label trueLabel, gen::Label falseLabel) const
{
    auto zero = gen::loadZero(type);
    auto cond = gen::cond(gen::NE, loadValue(), zero);
    gen::jmp(cond, trueLabel, falseLabel);
}

// for debugging and educational purposes
void
ConditionalExpr::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    std::cerr << "ConditionalExpr [ " << type << " ] " << std::endl;
    cond->print(indent + 4);
    thenExpr->print(indent + 4);
    elseExpr->print(indent + 4);
}
    

