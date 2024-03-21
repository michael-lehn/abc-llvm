#include <iostream>
#include <iomanip>

#include "gen/constant.hpp"
#include "gen/instruction.hpp"
#include "gen/variable.hpp"
#include "lexer/error.hpp"

#include "member.hpp"

namespace abc {

Member::Member(ExprPtr &&structure, UStr member, const Type *type,
	       std::size_t index, lexer::Loc loc)
    : Expr{loc, type}, structure{std::move(structure)}, member{member}
    , index{index}
{
}

ExprPtr
Member::create(ExprPtr &&structure, UStr member, lexer::Loc loc)
{
    assert(structure);
    assert(structure->type);
    auto structureType = structure->type->isPointer()
	? structure->type->refType()
	: structure->type;

    const Type *type = nullptr;
    std::size_t index = 0;
    if (structureType->isStruct() && structureType->hasSize()) {
	const auto &memberName = structureType->memberName();
	const auto &memberType = structureType->memberType();
	for (std::size_t i = 0; i < memberName.size(); ++i) {
	    if (member == memberName[i]) {
		type = memberType[i];
		index = i;
		break;
	    }
	}
    } else if (structureType->isStruct() && !structureType->hasSize()) {
	error::location(loc);
	error::out() << loc << ": error: " << structureType
	    << " is an incomplete struct\n";
	error::fatal();
	return nullptr;
    } else {
	error::location(loc);
	error::out() << loc << ": error: type " << structureType
	    << " is not a struct\n";
	error::fatal();
	return nullptr;
    }
    if (!type) {
	error::location(loc);
	error::out() << loc << ": error: " << structure << " of type "
	    << structure->type << " has no member " << member << "\n";
	if (structureType->isStruct()) {
	    error::out() << "members are:\n";
	    const auto &memberName = structureType->memberName();
	    const auto &memberType = structureType->memberType();
	    for (std::size_t i = 0; i < memberName.size(); ++i) {
		error::out() << memberName[i] << " of type " << memberType[i]
		    << "\n";
	    }
	}
	error::fatal();
	return nullptr;
    } else {
	auto p = new Member{std::move(structure), member, type, index, loc};
	return std::unique_ptr<Member>{p};
    }
}

bool
Member::hasAddress() const
{
    assert(type);
    return true;
}

bool
Member::isLValue() const
{
    return true;
}

bool
Member::isConst() const
{
    return false;
}

// for code generation
gen::Constant
Member::loadConstant() const
{
    assert(0);
    return nullptr;
}

gen::Value
Member::loadValue() const
{
    assert(type);
    return gen::fetch(loadAddress(), type);
}

gen::Value
Member::loadAddress() const
{
    auto structureType = structure->type->isPointer()
	? structure->type->refType()
	: structure->type;
    auto structureAddress = structure->type->isPointer()
	? structure->loadValue()
	: structure->loadAddress();
    return gen::pointerToIndex(structureType, structureAddress, index);
}

// for debugging and educational purposes
void
Member::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    std::cerr << structure
	<< (structure->type->isPointer() ? "->" : ".") << member
	<< " [ " << type << " ] " << std::endl;
}
    
void
Member::printFlat(std::ostream &out, int prec) const
{
    assert(structure);
    assert(structure->type);
    out << structure << (structure->type->isPointer() ? "->" : ".") << member;
}

} // namespace abc
