#include <charconv>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "error.hpp"
#include "gen.hpp"
#include "integerliteral.hpp"

IntegerLiteral::IntegerLiteral(UStr val, std::uint8_t  radix, const Type *type,
			       Token::Loc loc)
    : Expr{loc, type}, val{val}, radix{radix}
{
}

static const Type *getIntType(const char *s, const char *end,
			      std::uint8_t radix, Token::Loc loc);

ExprPtr
IntegerLiteral::create(UStr val, std::uint8_t  radix, const Type *type,
		       Token::Loc loc)
{
    if (!type) {
	type = getIntType(val.c_str(), val.c_str() + val.length(), radix, loc);
    }
    auto p = new IntegerLiteral{val, radix, type, loc};
    return std::unique_ptr<IntegerLiteral>{p};
}

ExprPtr
IntegerLiteral::create(std::int64_t val, const Type *type, Token::Loc loc)
{
    if (!type) {
	type = Type::getSignedInteger(64);
    }
    std::stringstream ss;
    ss << val;
    auto p = new IntegerLiteral{ss.str(), 10, type, loc};
    return std::unique_ptr<IntegerLiteral>{p};
}

bool
IntegerLiteral::hasAddr() const
{
    return false;
}

bool
IntegerLiteral::isLValue() const
{
    return false;
}

bool
IntegerLiteral::isConst() const
{
    return true;
}

// for code generation
gen::ConstVal
IntegerLiteral::loadConstValue() const
{
    return gen::loadIntConst(val.c_str(), type, radix);
}

gen::Reg
IntegerLiteral::loadValue() const
{
    return loadConstValue();
}


gen::Reg
IntegerLiteral::loadAddr() const
{
    assert(0 && "IntegerLiteral has no address");
    return nullptr;
}

void
IntegerLiteral::condJmp(gen::Label trueLabel, gen::Label falseLabel) const
{
    auto zero = gen::loadZero(type);
    auto cond = gen::cond(gen::NE, loadValue(), zero);
    gen::jmp(cond, trueLabel, falseLabel);
}

// for debugging and educational purposes
void
IntegerLiteral::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    if (radix != 10) {
	std::cerr << radix << "'";
    }
    std::cerr << val.c_str() << " [ " << type << " ] " << std::endl;
}

/*
 * auxiliary functions
 */

template <typename IntType, std::uint8_t numBits>
static bool
getIntType(const char *s, const char *end, std::uint8_t radix, const Type *&ty)
{
    IntType result;
    auto [ptr, ec] = std::from_chars(s, end, result, radix);
    if (ec == std::errc{}) {
	ty = Type::getSignedInteger(numBits);
	return true;
    }
    ty = nullptr; 
    return false;
}

static const Type *
getIntType(const char *s, const char *end, std::uint8_t radix, Token::Loc loc)
{
    //TODO: Handle type of integer literals like in Rust. Currently handled
    //	    like in C, i.e. it is at least of type 'int' (where i32 is choosen)
    const Type *ty = nullptr;
    if (getIntType<std::int32_t, 32>(s, end, radix, ty)
     || getIntType<std::int64_t, 64>(s, end, radix, ty))
    {
	return ty;
    }
    error::out() << loc << ": warning: signed integer '" << s
	<< "' does not fit into 64 bits"
	<< std::endl;
    error::warning();
    return Type::getSignedInteger(64);
}
