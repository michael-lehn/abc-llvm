#include <iostream>
#include <iomanip>

#include "gen/constant.hpp"
#include "gen/instruction.hpp"
#include "gen/variable.hpp"
#include "lexer/error.hpp"

#include "identifier.hpp"

namespace abc {

Identifier::Identifier(UStr name, UStr id, const Type *type, lexer::Loc loc)
    : Expr{loc, type}, name{name}, id{id}
{
}

ExprPtr
Identifier::create(UStr name, UStr id, const Type *type, lexer::Loc loc)
{
    assert(type);
    auto p = new Identifier{name, id, type, loc};
    return std::unique_ptr<Identifier>{p};
}

bool
Identifier::hasConstantAddress() const
{
    return gen::hasConstantAddress(id.c_str());
}

bool
Identifier::hasAddress() const
{
    assert(type);
    return type->isFunction() || type->hasSize();
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
    return gen::fetch(loadAddress(), type, false);
}

gen::Constant
Identifier::loadConstantAddress() const
{
    assert(hasConstantAddress());
    assert(id.c_str());
    return gen::loadConstantAddress(id.c_str());
}

gen::Value
Identifier::loadAddress() const
{
    assert(hasAddress());
    assert(id.c_str());
    return gen::loadAddress(id.c_str());
}

// for debugging and educational purposes
void
Identifier::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    std::cerr << name << " [ " << type << " ] " << std::endl;
}
    
void
Identifier::printFlat(std::ostream &out, int prec) const
{
    out << name;
}

} // namespace abc
