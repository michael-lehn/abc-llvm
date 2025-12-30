#include <iomanip>
#include <iostream>

#include "gen/constant.hpp"
#include "type/integertype.hpp"

#include "characterliteral.hpp"

//------------------------------------------------------------------------------

namespace abc {

CharacterLiteral::CharacterLiteral(unsigned processedVal, UStr val,
                                   lexer::Loc loc)
    : Expr{loc, IntegerType::createChar()}, processedVal{processedVal}, val{val}
{
}

ExprPtr
CharacterLiteral::create(UStr processedVal, UStr val, lexer::Loc loc)
{
    assert(processedVal.c_str());
    assert(processedVal.length() == 1);
    assert(val.c_str());
    assert(val.length());
    unsigned pval = *processedVal.c_str();
    auto p = new CharacterLiteral{pval, val, loc};
    return std::unique_ptr<CharacterLiteral>{p};
}

bool
CharacterLiteral::hasAddress() const
{
    return false;
}

bool
CharacterLiteral::isLValue() const
{
    return false;
}

bool
CharacterLiteral::isConst() const
{
    return true;
}

// for code generation
gen::Constant
CharacterLiteral::loadConstant() const
{
    return gen::getConstantInt(processedVal, type);
}

gen::Value
CharacterLiteral::loadValue() const
{
    return loadConstant();
}

gen::Value
CharacterLiteral::loadAddress() const
{
    assert(0 && "CharacterLiteral has no address");
    return nullptr;
}

// for debugging and educational purposes
void
CharacterLiteral::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    std::cerr << val << " [ " << type << " ] " << std::endl;
}

void
CharacterLiteral::printFlat(std::ostream &out, int prec) const
{
    out << val;
}

} // namespace abc
