#include <iomanip>
#include <iostream>

#include "gen/constant.hpp"
#include "gen/instruction.hpp"
#include "type/nullptrtype.hpp"

#include "nullptr.hpp"

namespace abc {

Nullptr::Nullptr(lexer::Loc loc) : Expr{loc, NullptrType::create()} {}

ExprPtr
Nullptr::create(lexer::Loc loc)
{
    auto p = new Nullptr{loc};
    return std::unique_ptr<Nullptr>{p};
}

bool
Nullptr::hasAddress() const
{
    return false;
}

bool
Nullptr::isLValue() const
{
    return false;
}

bool
Nullptr::isConst() const
{
    return true;
}

// for code generation
gen::Constant
Nullptr::loadConstant() const
{
    return gen::getConstantZero(type);
}

gen::Value
Nullptr::loadValue() const
{
    return loadConstant();
}

gen::Value
Nullptr::loadAddress() const
{
    assert(0 && "Nullptr has no address");
    return nullptr;
}

void
Nullptr::condition(gen::Label, gen::Label falseLabel) const
{
    gen::jumpInstruction(falseLabel);
}

// for debugging and educational purposes
void
Nullptr::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    std::cerr << "nullptr" << std::endl;
}

void
Nullptr::printFlat(std::ostream &out, int prec) const
{
    out << "nullptr";
}

} // namespace abc
