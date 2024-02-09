#include <iomanip>
#include <iostream>

#include "gen.hpp"
#include "stringliteral.hpp"

StringLiteral::StringLiteral(UStr val, const char *ident, std::size_t padding,
			     Token::Loc loc)
    : Expr{loc, Type::getString(val.length() + 1)}, val{val}, ident{ident}
    , padding{padding}
{
}

ExprPtr
StringLiteral::create(UStr val, UStr ident, Token::Loc loc)
{
    auto p = new StringLiteral{val, ident.c_str(), 0, loc};
    return std::unique_ptr<StringLiteral>{p};
}

ExprPtr
StringLiteral::create(UStr val, std::size_t padding, Token::Loc loc)
{
    auto p = new StringLiteral{val, nullptr, padding, loc};
    return std::unique_ptr<StringLiteral>{p};
}

bool
StringLiteral::hasAddr() const
{
    return ident;
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
    for (std::size_t i = 0; i < padding; ++i) {
	str.push_back(gen::loadIntConst(0, Type::getChar()));
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
    return gen::loadAddr(ident);
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
    std::cerr << val.c_str() << " [ string literal (" << type << ") ] "
	<< std::endl;
}

