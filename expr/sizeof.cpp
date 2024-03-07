#include <charconv>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "gen/constant.hpp"
#include "gen/instruction.hpp"
#include "gen/gentype.hpp"
#include "lexer/error.hpp"
#include "type/integertype.hpp"

#include "sizeof.hpp"

namespace abc {

Sizeof::Sizeof(const Type *sizeofType, ExprPtr &&sizeofExpr, lexer::Loc loc)
    : Expr{loc, IntegerType::createSizeType()}, sizeofType{sizeofType}
    , sizeofExpr{std::move(sizeofExpr)}
{
}

ExprPtr
Sizeof::create(const Type *sizeofType, lexer::Loc loc)
{
    assert(sizeofType);
    auto p = new Sizeof{sizeofType, nullptr, loc};
    return std::unique_ptr<Sizeof>{p};
}

ExprPtr
Sizeof::create(ExprPtr &&sizeofExpr, lexer::Loc loc)
{
    assert(sizeofExpr);
    assert(sizeofExpr->type);
    auto p = new Sizeof{nullptr, std::move(sizeofExpr), loc};
    return std::unique_ptr<Sizeof>{p};
}

bool
Sizeof::hasAddress() const
{
    return false;
}

bool
Sizeof::isLValue() const
{
    return false;
}

bool
Sizeof::isConst() const
{
    return true;
}

// for code generation
gen::Constant
Sizeof::loadConstant() const
{
    auto size =  gen::getSizeof(sizeofType ? sizeofType : sizeofExpr->type);
    return gen::getConstantInt(size, IntegerType::createSizeType());
}

gen::Value
Sizeof::loadValue() const
{
    return loadConstant();
}

gen::Value
Sizeof::loadAddress() const
{
    assert(0 && "Sizeof has no address");
    return nullptr;
}

void
Sizeof::condition(gen::Label trueLabel, gen::Label falseLabel) const
{
    auto zero = gen::getConstantZero(type);
    auto cond = gen::instruction(gen::NE, loadValue(), zero);
    gen::jumpInstruction(cond, trueLabel, falseLabel);
}

// for debugging and educational purposes
void
Sizeof::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    std::cerr << "sizeof ";
    if (sizeofType) {
	std::cerr << sizeofType;
    } else {
	std::cerr << sizeofExpr;
    }
    std::cerr << " [ " << type << " ] " << std::endl;
}

void
Sizeof::printFlat(std::ostream &out, int prec) const
{
    out << "sizeof(";
    if (sizeofType) {
	out << sizeofType;
    } else {
	out << sizeofExpr;
    }
    out << ")";
}

} // namespace abc
