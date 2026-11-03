#include <iomanip>
#include <iostream>

#include "gen/constant.hpp"
#include "gen/gentype.hpp"
#include "lexer/error.hpp"
#include "type/integertype.hpp"

#include "sizeof.hpp"

namespace abc {

static void
checkHasSize(const Type *type, lexer::Loc loc)
{
    if (!type->hasSize() && !type->isNullptr()) {
	error::location(loc);
	error::out() << error::setColor(error::BOLD) << loc << ": "
	             << error::setColor(error::BOLD_RED)
	             << "error: " << error::setColor(error::BOLD)
	             << "sizeof can not be applied to type '" << type << "'\n"
	             << error::setColor(error::NORMAL);
	error::fatal();
    }
}

Sizeof::Sizeof(const Type *sizeofType, ExprPtr &&sizeofExpr, lexer::Loc loc)
    : Expr{loc, IntegerType::createSizeType()}, sizeofType{sizeofType},
      sizeofExpr{std::move(sizeofExpr)}
{
}

ExprPtr
Sizeof::create(const Type *sizeofType, lexer::Loc loc)
{
    assert(sizeofType);
    checkHasSize(sizeofType, loc);
    auto p = new Sizeof{sizeofType, nullptr, loc};
    return std::unique_ptr<Sizeof>{p};
}

ExprPtr
Sizeof::create(ExprPtr &&sizeofExpr, lexer::Loc loc)
{
    assert(sizeofExpr);
    assert(sizeofExpr->type);
    checkHasSize(sizeofExpr->type, loc);
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
    auto size = gen::getSizeof(sizeofType ? sizeofType : sizeofExpr->type);
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
