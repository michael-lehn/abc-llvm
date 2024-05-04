#include <charconv>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "gen/constant.hpp"
#include "gen/instruction.hpp"
#include "lexer/error.hpp"
#include "type/integertype.hpp"

#include "floatliteral.hpp"

namespace abc {

FloatLiteral::FloatLiteral(UStr val, const Type *type, lexer::Loc loc)
    : Expr{loc, type}, val{val}
{
}

ExprPtr
FloatLiteral::create(UStr val, const Type *type, lexer::Loc loc)
{
    assert(val.c_str());
    assert(val.length());
    auto p = new FloatLiteral{val, type, loc};
    return std::unique_ptr<FloatLiteral>{p};
}

ExprPtr
FloatLiteral::create(double val, const Type *type, lexer::Loc loc)
{
    std::stringstream ss;
    ss << val;
    auto p = new FloatLiteral{UStr::create(ss.str()), type, loc};
    return std::unique_ptr<FloatLiteral>{p};
}

bool
FloatLiteral::hasAddress() const
{
    return false;
}

bool
FloatLiteral::isLValue() const
{
    return false;
}

bool
FloatLiteral::isConst() const
{
    return true;
}

// for code generation
gen::Constant
FloatLiteral::loadConstant() const
{
    return gen::getConstantFloat(val.c_str(), type);
}

gen::Value
FloatLiteral::loadValue() const
{
    return loadConstant();
}

gen::Value
FloatLiteral::loadAddress() const
{
    assert(0 && "FloatLiteral has no address");
    return nullptr;
}

// for debugging and educational purposes
void
FloatLiteral::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    std::cerr << val << " [ " << type << " ] " << std::endl;
}

void
FloatLiteral::printFlat(std::ostream &out, int prec) const
{
    out << val;
}

} // namespace abc
