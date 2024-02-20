#include <iostream>

#include "castexpr.hpp"
#include "error.hpp"
#include "expr.hpp"
#include "integerliteral.hpp"
#include "initializerlist.hpp"

InitializerList::InitializerList(const Type *type)
    : type_{type}, value{type->getNumMembers()}, valueLoc{type->getNumMembers()}
{
}

const Type *
InitializerList::type() const
{
    return type_;
}

bool
InitializerList::isConst() const
{
    for (std::size_t i = 0; i < value.size(); ++i) {
	const auto &v = value[i];
	if (std::holds_alternative<ExprPtr>(v)) {
	    const auto &e = std::get<ExprPtr>(v);
	    if (e && !e->isConst()) {
		return false;
	    }
	} else if (std::holds_alternative<InitializerList>(v)) {
	    if (!std::get<InitializerList>(v).isConst()) {
		return false;
	    }
	}
    }
    return true;
}

void
InitializerList::add(ExprPtr &&expr)
{
    auto ty = type()->getMemberType(pos);
    expr = CastExpr::create(std::move(expr), ty, expr->loc);
    assert(pos < type()->getNumMembers());
    valueLoc[pos] = expr->loc;
    value[pos++] = std::move(expr);
}

void
InitializerList::add(InitializerList &&initList, Token::Loc loc)
{
    valueLoc[pos] = loc;
    value[pos++] = std::move(initList);
}

void
InitializerList::store(gen::Reg addr) const
{
    if (type()->isStruct() || type()->isArray()) {
	std::vector<gen::ConstVal> val{value.size()};
	for (size_t i = 0; i < value.size(); ++i) {
	    auto memAddr = gen::ptrMember(type(), addr, i);
	    store(i, memAddr);
	}
    } else {
	assert(value.size() == 1);
	store(0, addr);
    }
}

void
InitializerList::store(std::size_t index, gen::Reg addr) const
{
    auto &v = value[index];
    auto ty = type()->getMemberType(index);
    if (std::holds_alternative<ExprPtr>(v)) {
	const auto &e = std::get<ExprPtr>(v);
	auto val = e
	    ? gen::cast(e->loadValue(), e->type, ty)
	    : gen::loadZero(ty);
	gen::store(val, addr, ty);
    } else if (std::holds_alternative<InitializerList>(v)) {
	const auto &v = std::get<InitializerList>(value[index]);
	v.store(addr);
    }
}

gen::ConstVal
InitializerList::loadConstValue() const
{
    if (type()->isArray() || type()->isStruct()) {
	std::vector<gen::ConstVal> val{value.size()};
	for (size_t i = 0; i < value.size(); ++i) {
	    val[i] = loadConstValue(i);
	}
	if (type()->isArray()) {
	    return gen::loadConstArray(val, type());
	} else {
	    return gen::loadConstStruct(val, type());
	}
    } else {
	assert(value.size() == 1);
	return loadConstValue(0);
    }
}

gen::ConstVal
InitializerList::loadConstValue(size_t index) const
{
    auto &v = value[index];
    auto ty = type()->getMemberType(index);
    if (std::holds_alternative<ExprPtr>(v)) {
	const auto &e = std::get<ExprPtr>(v);
	if (e) {
	    if (!e->isConst()) {
		error::out() << e->loc << ": error: not const" << std::endl;
		error::fatal();
		return gen::loadZero(e->type);
	    }
	    return gen::cast(e->loadConstValue(), e->type, ty);
	} else {
	    return gen::loadZero(ty);
	}
    } else if (std::holds_alternative<InitializerList>(v)) {
	const auto &v = std::get<InitializerList>(value[index]);
	return v.loadConstValue();
    }
    assert(0);
    return nullptr;
}

void
InitializerList::print(int indent) const
{
    error::out(indent) << "InitializerList {" << std::endl;
    for (std::size_t i = 0; i < value.size(); ++i) {
	const auto &v = value[i];
	if (std::holds_alternative<ExprPtr>(v)) {
	    const auto &e = std::get<ExprPtr>(v);
	    if (e) {
		e->print(indent + 4);
	    } else {
		std::printf("%*sZERO\n", indent + 4, "");
	    }
	} else if (std::holds_alternative<InitializerList>(v)) {
	    std::get<InitializerList>(v).print(indent + 4);
	}
    }
    error::out(indent) << "}" << std::endl;
}

void
InitializerList::printFlat(std::ostream &out, bool isFactor) const
{
    if (value.size() > 1) {
	out << "{";
    }
    for (std::size_t i = 0; i < value.size(); ++i) {
	const auto &v = value[i];
	if (std::holds_alternative<ExprPtr>(v)) {
	    const auto &e = std::get<ExprPtr>(v);
	    if (e) {
		e->printFlat(out, isFactor);
	    } else {
		out << "0";
	    }
	} else if (std::holds_alternative<InitializerList>(v)) {
	    std::get<InitializerList>(v).printFlat(out, isFactor);
	}
	if (i + 1 < value.size()) {
	    out << ", ";
	}
    }
    if (value.size() > 1) {
	out << "}";
    }
}
