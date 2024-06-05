#include <iomanip>
#include <iostream>
#include <sstream>

#include "gen/constant.hpp"
#include "gen/function.hpp"
#include "gen/instruction.hpp"
#include "gen/label.hpp"
#include "gen/variable.hpp"
#include "lexer/error.hpp"
#include "type/functiontype.hpp"
#include "type/integertype.hpp"

#include "assertexpr.hpp"
#include "implicitcast.hpp"
#include "promotion.hpp"

namespace abc {

static UStr assertFnName;
static const Type *assertFnType;

AssertExpr::AssertExpr(ExprPtr &&expr, lexer::Loc loc)
    : Expr{loc, IntegerType::createBool()}, expr{std::move(expr)}
{
}

ExprPtr
AssertExpr::create(ExprPtr &&expr, lexer::Loc loc)
{
    assert(expr);
    auto p = new AssertExpr{std::move(expr), loc};
    return std::unique_ptr<AssertExpr>{p};
}

void
AssertExpr::setFunction(UStr name, const Type *fnType)
{
    assertFnName = name;
    assertFnType = fnType;
}

void
AssertExpr::apply(std::function<bool(const Expr *)> op) const
{
    if (op(this)) {
	expr->apply(op);
    }
}

bool
AssertExpr::hasAddress() const
{
    return false;
}

bool
AssertExpr::isLValue() const
{
    return false;
}

bool
AssertExpr::isConst() const
{
    return false;
}

// for code generation
gen::Constant
AssertExpr::loadConstant() const
{
    assert(isConst());
    return nullptr;
}

gen::Value
AssertExpr::loadValue() const
{
    assert(assertFnType);
    assert(assertFnName.c_str());

    auto trueLabel = gen::getLabel("true");
    auto falseLabel = gen::getLabel("false");

    expr->condition(trueLabel, falseLabel);
    gen::defineLabel(falseLabel);

    std::vector<gen::Value> argValue;

    std::stringstream ss;
    bool old = ImplicitCast::setOutput(false);
    ss << expr;
    ImplicitCast::setOutput(old);
    argValue.push_back(gen::loadStringAddress(ss.str().c_str()));
    argValue.push_back(gen::loadStringAddress(loc.path.c_str()));
    argValue.push_back(gen::getConstantInt(loc.from.line,
					   IntegerType::createInt()));
    
    // argValue.push_back(a->loadValue());
    auto fnAddr = gen::loadAddress(assertFnName.c_str());
    gen::functionCall(fnAddr, assertFnType, argValue);

    gen::jumpInstruction(trueLabel);
    gen::defineLabel(trueLabel);
    
    return gen::getFalse();
}

gen::Value
AssertExpr::loadAddress() const
{
    assert(0 && "AssertExpr has no address");
    return nullptr;
}

void
AssertExpr::condition(gen::Label trueLabel, gen::Label falseLabel) const
{
    auto zero = gen::getConstantZero(type);
    auto cond = gen::instruction(gen::NE, loadValue(), zero);
    gen::jumpInstruction(cond, trueLabel, falseLabel);
}

// for debugging and educational purposes
void
AssertExpr::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    std::cerr << "assert (" << expr << ") [ " << type << " ] " << std::endl;
}

void
AssertExpr::printFlat(std::ostream &out, int prec) const
{
    out << "assert(" << expr << ")";
}

} // namespace abc
