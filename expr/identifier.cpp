#include <iostream>
#include <iomanip>

#include "gen/gen.hpp"
#include "gen/variable.hpp"
#include "lexer/error.hpp"

#include "identifier.hpp"

namespace abc {

Identifier::Identifier(UStr ident, const Type *type, lexer::Loc loc)
    : Expr{loc, type}, ident{ident}
{
}

ExprPtr
Identifier::create(UStr ident, const Type *type, lexer::Loc loc)
{
    assert(type);
    auto p = new Identifier{ident, type, loc};
    return std::unique_ptr<Identifier>{p};
}

bool
Identifier::hasAddr() const
{
    assert(type);
    return type->hasSize();
}

bool
Identifier::isLValue() const
{
    return type;
}

bool
Identifier::isConst() const
{
    return false;
}

// for code generation
gen::Constant
Identifier::loadConstant() const
{
    assert(0);
    return nullptr;
}

gen::Value
Identifier::loadValue() const
{
    assert(type);
    if (type->isFunction()) {
	return loadAddress();
    }
    assert(ident.c_str());
    return gen::fetch(loadAddress(), type);
}

gen::Value
Identifier::loadAddress() const
{
    assert(hasAddr());
    assert(ident.c_str());
    return gen::loadAddress(ident.c_str());
}

void
Identifier::condJmp(gen::Label trueLabel, gen::Label falseLabel) const
{
    assert(0 && "Not implemented");
    /*
    auto zero = gen::loadZero(type);
    auto cond = gen::cond(gen::NE, loadValue(), zero);
    gen::jmp(cond, trueLabel, falseLabel);
    */
}

// for debugging and educational purposes
void
Identifier::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    std::cerr << ident << " [ " << type << " ] " << std::endl;
}
    
void
Identifier::printFlat(std::ostream &out, int prec) const
{
    out << ident;
}

} // namespace abc
