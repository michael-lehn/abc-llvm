#include <charconv>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "gen/constant.hpp"
#include "gen/instruction.hpp"
#include "lexer/error.hpp"
#include "type/integertype.hpp"

#include "enumconstant.hpp"

//------------------------------------------------------------------------------

namespace abc {

EnumConstant::EnumConstant(UStr name, std::int64_t value, const Type *type,
			   lexer::Loc loc)
    : Expr{loc, type}, name{name}, value{value}
{
}

ExprPtr
EnumConstant::create(UStr name, std::int64_t value, const Type *type,
		     lexer::Loc loc)
{
    assert(!name.empty());
    auto p = new EnumConstant{name, value, type, loc};
    return std::unique_ptr<EnumConstant>{p};
}

bool
EnumConstant::hasAddress() const
{
    return false;
}

bool
EnumConstant::isLValue() const
{
    return false;
}

bool
EnumConstant::isConst() const
{
    return true;
}

// for code generation
gen::Constant
EnumConstant::loadConstant() const
{
    return gen::getConstantInt(value, type);
}

gen::Value
EnumConstant::loadValue() const
{
    return loadConstant();
}

gen::Value
EnumConstant::loadAddress() const
{
    assert(0 && "EnumConstant has no address");
    return nullptr;
}

// for debugging and educational purposes
void
EnumConstant::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    std::cerr << name << " [ " << value << " ] " << std::endl;
}

void
EnumConstant::printFlat(std::ostream &out, int prec) const
{
    out << name;
}

} // namespace abc
