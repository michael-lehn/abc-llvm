#include "expr.hpp"
#include "initializerlist.hpp"


InitializerList::InitializerList(const Type *type)
    : type{type}, pos{0}, value{type ? type->getNumMembers() : 0}
    , valueLoc{type ? type->getNumMembers() : 0}
{
}

const Type *
InitializerList::getType() const
{
    return type;
}

bool
InitializerList::isConst() const
{
    assert(type && "InitializerList::isConst() requires type");

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
InitializerList::setType(const Type *ty)
{
    assert(!type && "InitializerList::setType(): type alredy set");
    assert(ty && "InitializerList::setType(): ty is nullptr");
    type = ty;

    if (value.size() > type->getNumMembers()) {
	error::out() << valueLoc[type->getNumMembers()]
	    << ": excess elements in struct initializer" << std::endl;
	error::fatal();
    }

    valueType.resize(type->getNumMembers());
    valueLoc.resize(valueType.size());

    for (std::size_t i = 0; i < type->getNumMembers(); ++i) {
	valueType[i] = type->getMemberType(pos);
	auto &v = value[i];
	if (std::holds_alternative<InitializerList>(v)) {
	    std::get<InitializerList>(v).setType(valueType[i]);
	}
    }
}

void
InitializerList::add(ExprPtr &&expr, Token::Loc loc)
{
    if (!type) {
	valueLoc.resize(valueLoc.size()  + 1);
	value.resize(value.size()  + 1);
    } else {
	auto ty = type->getMemberType(pos);
	expr = Expr::createCast(std::move(expr), ty, expr->getLoc());
    }
    valueLoc[pos] = loc;
    value[pos++] = std::move(expr);
}

void
InitializerList::add(InitializerList &&initList, Token::Loc loc)
{
    if (!type) {
	valueLoc.resize(valueLoc.size()  + 1);
	value.resize(value.size()  + 1);
    }

    valueLoc[pos] = loc;
    value[pos++] = std::move(initList);
}

void
InitializerList::store(gen::Reg addr) const
{
    assert(type && "InitializerList::store(addr): no type");

    if (type->isStruct() || type->isArray()) {
	std::vector<gen::ConstVal> val{value.size()};
	for (size_t i = 0; i < value.size(); ++i) {
	    auto memAddr = gen::ptrMember(type, addr, i);
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
    assert(type && "InitializerList::store(index, addr): no type");

    auto &v = value[index];
    auto ty = type->getMemberType(index);
    if (std::holds_alternative<ExprPtr>(v)) {
	const auto &e = std::get<ExprPtr>(v);
	auto val = e ? e->loadValue() : gen::loadZero(ty);
	val = gen::cast(val, e->getType(), ty);
	gen::store(val, addr, ty);
    } else if (std::holds_alternative<InitializerList>(v)) {
	const auto &v = std::get<InitializerList>(value[index]);
	v.store(addr);
    }
}

gen::ConstVal
InitializerList::loadConst() const
{
    assert(isConst() && "InitializerList::load(): not const");
    assert(type && "InitializerList::load(): type is nullptr");

    if (type->isArray() || type->isStruct()) {
	std::vector<gen::ConstVal> val{value.size()};
	for (size_t i = 0; i < value.size(); ++i) {
	    val[i] = loadConst(i);
	}
	if (type->isArray()) {
	    return gen::loadConstArray(val, type);
	} else {
	    return gen::loadConstStruct(val, type);
	}
    } else {
	assert(value.size() == 1);
	return loadConst(0);
    }
}

gen::ConstVal
InitializerList::loadConst(size_t index) const
{
    assert(type);

    auto &v = value[index];
    auto ty = type->getMemberType(index);
    if (std::holds_alternative<ExprPtr>(v)) {
	const auto &e = std::get<ExprPtr>(v);
	if (e) {
	    return gen::cast(e->loadConst(), e->getType(), ty);
	} else {
	    return gen::loadZero(ty);
	}
    } else if (std::holds_alternative<InitializerList>(v)) {
	const auto &v = std::get<InitializerList>(value[index]);
	return v.loadConst();
    }
    assert(0);
    return nullptr;
}

void
InitializerList::print(int indent) const
{
    for (std::size_t i = 0; i < value.size(); ++i) {
	const auto &v = value[i];
	if (std::holds_alternative<ExprPtr>(v)) {
	    const auto &e = std::get<ExprPtr>(v);
	    if (e) {
		e->print(indent + 4);
	    } else {
		std::printf("%*sZER0\n", indent + 4, "");
	    }
	} else if (std::holds_alternative<InitializerList>(v)) {
	    std::get<InitializerList>(v).print(indent + 4);
	}
    }
}

