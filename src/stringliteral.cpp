#include <iomanip>
#include <iostream>

#include "gen.hpp"
#include "stringliteral.hpp"
#include "symtab.hpp"

StringLiteral::StringLiteral(UStr val, UStr valRaw, Token::Loc loc)
    : Expr{loc, Type::getString(val.length())}, val{val}, valRaw{valRaw}
    , ident{Symtab::addStringLiteral(val)}
{
}

ExprPtr
StringLiteral::create(UStr val, UStr valRaw, Token::Loc loc)
{
    auto p = new StringLiteral{val, valRaw, loc};
    return std::unique_ptr<StringLiteral>{p};
}

bool
StringLiteral::hasAddr() const
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
gen::ConstVal
StringLiteral::loadConstValue() const
{
    std::vector<gen::ConstVal> str;
    for (std::size_t i = 0; i < val.length(); ++i) {
	auto ch = val.c_str()[i];
	str.push_back(gen::loadIntConst(ch, Type::getChar()));
    }
    str.push_back(gen::loadIntConst(0, Type::getChar()));
    return gen::loadConstArray(str, Type::getString(val.length()));
}

gen::Reg
StringLiteral::loadValue() const
{
    return loadConstValue();
}

gen::Reg
StringLiteral::loadAddr() const
{
    assert(hasAddr());
    return gen::loadAddr(ident.c_str());
}

void
StringLiteral::condJmp(gen::Label trueLabel, gen::Label falseLabel) const
{
    auto zero = gen::loadZero(type);
    auto cond = gen::cond(gen::NE, loadValue(), zero);
    gen::jmp(cond, trueLabel, falseLabel);
}

// for debugging and educational purposes
void
StringLiteral::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    std::cerr << valRaw.c_str() << " [ string literal (" << type << ") ] "
	<< std::endl;
}

void
StringLiteral::printFlat(std::ostream &out, int prec) const
{
    out << valRaw.c_str();
}
