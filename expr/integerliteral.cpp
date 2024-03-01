#include <charconv>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "gen/constant.hpp"
#include "gen/instruction.hpp"
#include "integerliteral.hpp"
#include "lexer/error.hpp"
#include "type/integertype.hpp"

static const abc::Type *getIntType(const char *s, const char *end,
			           std::uint8_t radix, abc::lexer::Loc loc);

//------------------------------------------------------------------------------

namespace abc {

IntegerLiteral::IntegerLiteral(UStr val, std::uint8_t  radix, const Type *type,
			       lexer::Loc loc)
    : Expr{loc, type}, val{val}, radix{radix}
{
}

ExprPtr
IntegerLiteral::create(UStr val, std::uint8_t  radix, const Type *type,
		       lexer::Loc loc)
{
    assert(val.c_str());
    assert(val.length());
    if (!type) {
	type = getIntType(val.c_str(), val.c_str() + val.length(), radix, loc);
    }
    auto p = new IntegerLiteral{val, radix, type, loc};
    return std::unique_ptr<IntegerLiteral>{p};
}

ExprPtr
IntegerLiteral::create(std::int64_t val, const Type *type, lexer::Loc loc)
{
    if (!type) {
	type = IntegerType::createSigned(64);
    }
    std::stringstream ss;
    ss << val;
    auto p = new IntegerLiteral{UStr::create(ss.str()), 10, type, loc};
    return std::unique_ptr<IntegerLiteral>{p};
}

bool
IntegerLiteral::hasAddress() const
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
gen::Constant
IntegerLiteral::loadConstant() const
{
    return gen::getConstantInt(val.c_str(), type, radix);
}

gen::Value
IntegerLiteral::loadValue() const
{
    return loadConstant();
}

gen::Value
IntegerLiteral::loadAddress() const
{
    assert(0 && "IntegerLiteral has no address");
    return nullptr;
}

void
IntegerLiteral::condition(gen::Label trueLabel, gen::Label falseLabel) const
{
    auto zero = gen::getConstantZero(type);
    auto cond = gen::instruction(gen::NE, loadValue(), zero);
    gen::jumpInstruction(cond, trueLabel, falseLabel);
}

// for debugging and educational purposes
void
IntegerLiteral::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    if (radix != 10) {
	std::cerr << int(radix) << "'";
    }
    std::cerr << val << " [ " << type << " ] " << std::endl;
}

void
IntegerLiteral::printFlat(std::ostream &out, int prec) const
{
    if (radix == 8) {
	if (val != UStr::create("0")) {
	    out << "0";
	}
    } else if (radix == 16) {
	out << "0x";
    } else if (radix != 10) {
	out << int(radix) << "'";
    }
    out << val;
}

} // namespace abc

//------------------------------------------------------------------------------

/*
 * auxiliary functions
 */

template <typename IntType, std::uint8_t numBits>
static bool
getIntType(const char *s, const char *end, std::uint8_t radix,
	   const abc::Type *&ty)
{
    IntType result;
    auto [ptr, ec] = std::from_chars(s, end, result, radix);
    if (ec == std::errc{}) {
	ty = abc::IntegerType::createSigned(numBits);
	return true;
    }
    ty = nullptr; 
    return false;
}

static const abc::Type *
getIntType(const char *s, const char *end, std::uint8_t radix,
	   abc::lexer::Loc loc)
{
    //TODO: Handle type of integer literals like in Rust. Currently handled
    //	    like in C, i.e. it is at least of type 'int' (where i32 is choosen)
    const abc::Type *ty = nullptr;
    if (getIntType<std::int32_t, 32>(s, end, radix, ty)
     || getIntType<std::int64_t, 64>(s, end, radix, ty))
    {
	return ty;
    }
    abc::error::out() << loc << ": warning: signed integer '" << s
	<< "' does not fit into 64 bits"
	<< std::endl;
    abc::error::warning();
    return abc::IntegerType::createSigned(64);
}
