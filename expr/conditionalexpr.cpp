#include <iomanip>
#include <iostream>

#include "gen/constant.hpp"
#include "gen/instruction.hpp"
#include "gen/label.hpp"
#include "lexer/error.hpp"
#include "type/pointertype.hpp"

#include "conditionalexpr.hpp"
#include "implicitcast.hpp"

namespace abc {

// The value of a constant condition. No instruction: this is also used outside of
// functions, and an address can not be compared there. An array is an address, so
// it is true, also a string literal without any character.
static bool
constantCondition(const Expr *cond)
{
    return cond->type->isArray() || !gen::isZero(cond->loadConstant());
}

ConditionalExpr::ConditionalExpr(ExprPtr cond, ExprPtr trueExpr,
                                 ExprPtr falseExpr, const Type *type,
                                 bool thenElseStyle, lexer::Loc loc)
    : Expr{loc, type}, cond{std::move(cond)}, trueExpr{std::move(trueExpr)},
      falseExpr{std::move(falseExpr)}, thenElseStyle{thenElseStyle}
{
}

ExprPtr
ConditionalExpr::create(ExprPtr cond, ExprPtr trueExpr, ExprPtr falseExpr,
                        bool thenElseStyle, lexer::Loc loc)
{
    assert(trueExpr->type);
    assert(falseExpr->type);
    auto type = Type::common(trueExpr->type, falseExpr->type);
    if (type && type->isFunction()) {
	error::location(loc);
	error::out() << error::setColor(error::BOLD) << loc << ": "
	             << error::setColor(error::BOLD_RED)
	             << "error: " << error::setColor(error::BOLD)
	             << "use '&' to get the address of a function\n"
	             << error::setColor(error::NORMAL);
	error::fatal();
	return nullptr;
    }
    if (!type) {
	error::location(loc);
	error::out() << error::setColor(error::BOLD) << loc << ": "
	             << ": " << error::setColor(error::BOLD_RED)
	             << "error: " << error::setColor(error::BOLD)
	             << "can not combine expressions of type '"
	             << trueExpr->type << "' and type '" << falseExpr->type
	             << "'\n"
	             << error::setColor(error::NORMAL);
	error::location(trueExpr->loc);
	error::out() << error::setColor(error::BOLD) << trueExpr->loc
	             << ": note: type '" << trueExpr->type << "'\n"
	             << error::setColor(error::NORMAL);
	error::location(falseExpr->loc);
	error::out() << error::setColor(error::BOLD) << falseExpr->loc
	             << ": note: type '" << falseExpr->type << "'\n"
	             << error::setColor(error::NORMAL);
	error::fatal();
	return nullptr;
    } else {
	trueExpr = ImplicitCast::create(std::move(trueExpr), type);
	falseExpr = ImplicitCast::create(std::move(falseExpr), type);
    }
    auto p = new ConditionalExpr{std::move(cond),      std::move(trueExpr),
                                 std::move(falseExpr), type,
                                 thenElseStyle,        loc};
    return std::unique_ptr<ConditionalExpr>{p};
}

bool
ConditionalExpr::hasAddress() const
{
    return trueExpr->hasAddress() && falseExpr->hasAddress();
}

bool
ConditionalExpr::isLValue() const
{
    return false;
}

bool
ConditionalExpr::isConst() const
{
    if (!cond->isConst()) {
	return false;
    }
    if (constantCondition(cond.get())) {
	return trueExpr->isConst();
    } else {
	return falseExpr->isConst();
    }
}

// for code generation
gen::Constant
ConditionalExpr::loadConstant() const
{
    assert(isConst());
    if (constantCondition(cond.get())) {
	return trueExpr->loadConstant();
    } else {
	return falseExpr->loadConstant();
    }
}

gen::Value
ConditionalExpr::loadValue() const
{
    assert(type);
    auto thenLabel = gen::getLabel("condTrue");
    auto elseLabel = gen::getLabel("condFalse");
    auto endLabel = gen::getLabel("end");

    cond->condition(thenLabel, elseLabel);
    gen::defineLabel(thenLabel);
    auto trueValue = trueExpr->loadValue();
    thenLabel = gen::jumpInstruction(endLabel); // update needed for phi
    gen::defineLabel(elseLabel);
    auto falseValue = falseExpr->loadValue();
    elseLabel = gen::jumpInstruction(endLabel); // update needed for phi
    gen::defineLabel(endLabel);
    if (type->isVoid()) {
	return nullptr; // calls of void functions: no value to select
    }
    return gen::phi(trueValue, thenLabel, falseValue, elseLabel, type);
}

gen::Value
ConditionalExpr::loadAddress() const
{
    assert(type);
    assert(hasAddress());
    auto thenLabel = gen::getLabel("condTrue");
    auto elseLabel = gen::getLabel("condFalse");
    auto endLabel = gen::getLabel("end");

    cond->condition(thenLabel, elseLabel);
    gen::defineLabel(thenLabel);
    auto trueValue = trueExpr->loadAddress();
    thenLabel = gen::jumpInstruction(endLabel); // update needed for phi
    gen::defineLabel(elseLabel);
    auto falseValue = falseExpr->loadAddress();
    elseLabel = gen::jumpInstruction(endLabel); // update needed for phi
    gen::defineLabel(endLabel);
    return gen::phi(trueValue, thenLabel, falseValue, elseLabel,
                    PointerType::create(type));
}

void
ConditionalExpr::condition(gen::Label trueLabel, gen::Label falseLabel) const
{
    if (type->isVoid() || !type->isScalar()) {
	Expr::condition(trueLabel, falseLabel); // reports the error
	return;
    }
    auto zero = gen::getConstantZero(type);
    auto cond = gen::instruction(gen::conditionOp(type), loadValue(), zero);
    gen::jumpInstruction(cond, trueLabel, falseLabel);
}

// for debugging and educational purposes
void
ConditionalExpr::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    std::cerr << "? .. : ..." << " [ " << type << " ] " << std::endl;
    trueExpr->print(indent + 4);
    falseExpr->print(indent + 4);
}

void
ConditionalExpr::printFlat(std::ostream &out, int prec) const
{
    if (thenElseStyle) {
	out << cond << " then " << trueExpr << " else " << falseExpr;
    } else {
	out << cond << " ? " << trueExpr << " : " << falseExpr;
    }
}

} // namespace abc
