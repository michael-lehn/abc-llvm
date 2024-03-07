#include <iostream>
#include <iomanip>

#include "gen/constant.hpp"
#include "gen/instruction.hpp"
#include "gen/variable.hpp"
#include "lexer/error.hpp"
#include "type/arraytype.hpp"
#include "type/integertype.hpp"

#include "stringliteral.hpp"

namespace abc {

StringLiteral::StringLiteral(const Type *type, UStr val, UStr valRaw,
			     lexer::Loc loc)
    : Expr{loc, type}, val{val}, valRaw{valRaw}
{
}

ExprPtr
StringLiteral::create(UStr val, UStr valRaw, lexer::Loc loc)
{
    auto type = ArrayType::create(IntegerType::createChar(), val.length() + 1);
    auto p = new StringLiteral{type, val, valRaw, loc};
    return std::unique_ptr<StringLiteral>{p};
}

bool
StringLiteral::hasAddress() const
{
    return true;
}

bool
StringLiteral::isLValue() const
{
    return false;
}

bool
StringLiteral::isConst() const
{
    return true;
}

// for code generation
gen::Constant
StringLiteral::loadConstant() const
{
    return gen::getString(val.c_str());
}

gen::Value
StringLiteral::loadValue() const
{
    return loadConstant();
}

gen::Value
StringLiteral::loadAddress() const
{
    assert(hasAddress());
    return gen::loadStringAddress(val.c_str());
}

void
StringLiteral::condition(gen::Label trueLabel, gen::Label) const
{
    gen::jumpInstruction(trueLabel);
}

// for debugging and educational purposes
void
StringLiteral::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    std::cerr << "\"" << val << "\" [ " << type << " ] " << std::endl;
}
    
void
StringLiteral::printFlat(std::ostream &out, int prec) const
{
    out << valRaw;
}

} // namespace abc
