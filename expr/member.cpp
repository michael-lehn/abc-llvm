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

    if (!structure->hasAddress()) {
	error::location(loc);
	error::out() << error::setColor(error::BOLD) << structure->loc
	    << ": " << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << structure << " has no address (internal compiler error)\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
	return nullptr;
    }

    const Type *type = nullptr;
    std::size_t index = 0;
    if (structureType->isStruct() && structureType->hasSize()) {
	const auto &memberName = structureType->memberName();
	const auto &memberIndex = structureType->memberIndex();
	const auto &memberType = structureType->memberType();
	for (std::size_t i = 0; i < memberName.size(); ++i) {
	    if (member == memberName[i]) {
		type = memberType[i];
		index = memberIndex[i];
		break;
	    }
	}
    } else if (structureType->isStruct() && !structureType->hasSize()) {
	error::location(loc);
	error::out() << error::setColor(error::BOLD) << loc << ": "
	    << ": " << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "'" << structureType
	    << "' is an incomplete struct\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
	return nullptr;
    } else {
	error::location(loc);
	error::out() << error::setColor(error::BOLD) << loc
	    << ": " << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << "type " << structureType << " is not a struct\n"
	    << error::setColor(error::NORMAL);
	error::fatal();
	return nullptr;
    }
    if (!type) {
	error::location(loc);
	error::out() << error::setColor(error::BOLD) << loc << ": "
	    << error::setColor(error::BOLD_RED) << "error: "
	    << error::setColor(error::BOLD)
	    << structure << " of type "
	    << structure->type << " has no member " << member << "\n"
	    << error::setColor(error::NORMAL);
	if (structureType->isStruct()) {
	    error::out() << error::setColor(error::BOLD) << "members are:\n";
	    const auto &memberName = structureType->memberName();
	    const auto &memberType = structureType->memberType();
	    for (std::size_t i = 0; i < memberName.size(); ++i) {
		error::out() << memberName[i] << " of type " << memberType[i]
		    << "\n";
	    }
	    error::out() << error::setColor(error::NORMAL);
	}
	error::fatal();
	return nullptr;
    } else {
	auto p = new Member{std::move(structure), member, type, index, loc};
	return std::unique_ptr<Member>{p};
    }
}

void
Member::apply(std::function<bool(const Expr *)> op) const
{
    if (op(this)) {
	structure->apply(op);
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
