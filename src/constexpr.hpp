#ifndef CONSTEXPR_HPP
#define CONSTEXPR_HPP

#include <cstdio>
#include <variant>
#include <vector>

#include "expr.hpp"
#include "gen.hpp"
#include "type.hpp"

class ConstExpr
{
    public:
	ConstExpr(const Type *type= nullptr)
	    : type{type}, pos{0}, value{type ? type->getNumMembers() : 0}
	    , valueLoc{type ? type->getNumMembers() : 0}

	{
	}

	const Type *getType() const
	{
	    return type;
	}

	// only use if type was not already set in constructor
	void setType(const Type *ty)
	{
	    assert(!type);
	    assert(ty);
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
		if (std::holds_alternative<ConstExpr>(v)) {
		    std::get<ConstExpr>(v).setType(valueType[i]);
		}
	    }
	}

	void add(ExprPtr &&expr, Token::Loc loc = Token::Loc{})
	{
	    assert(expr->isConst());
	    if (!type) {
		valueLoc.resize(valueLoc.size()  + 1);
		value.resize(value.size()  + 1);
	    }

	    if (type) {
		auto ty = type->getMemberType(pos);
		expr = Expr::createCast(std::move(expr), ty, expr->getLoc());
	    }
	    valueLoc[pos] = loc;
	    value[pos++] = std::move(expr);
	}

	void add(ConstExpr &&constExpr, Token::Loc loc = Token::Loc{})
	{
	    if (!type) {
		valueLoc.resize(valueLoc.size()  + 1);
		value.resize(value.size()  + 1);
	    }

	    valueLoc[pos] = loc;
	    value[pos++] = std::move(constExpr);
	}

	std::size_t size(void) const
	{
	    return value.size();
	}

	gen::ConstVal load(void) const
	{
	    assert(type);

	    if (type->isArray() || type->isStruct()) {
		std::vector<gen::ConstVal> val{size()};
		for (size_t i = 0; i < size(); ++i) {
		    val[i] = load(i);
		}
		if (type->isArray()) {
		    return gen::loadConstArray(val, type);
		} else {
		    return gen::loadConstStruct(val, type);
		}
	    } else {
		assert(size() == 1);
		return load(0);
	    }
	}

	gen::ConstVal load(size_t index) const
	{
	    assert(type);

	    auto &v = value[index];
	    auto ty = type->getMemberType(index);
	    if (std::holds_alternative<ExprPtr>(v)) {
		const auto &e = std::get<ExprPtr>(v);
		if (e) {
		    return e->loadConst();
		} else {
		    return gen::loadZero(ty);
		}
	    } else if (std::holds_alternative<ConstExpr>(v)) {
		const auto &v = std::get<ConstExpr>(value[index]);
		return v.load();
	    }
	    assert(0);
	    return nullptr;
	}

	void print(int indent = 0) const
	{
	    for (std::size_t i = 0; i < size(); ++i) {
		const auto &v = value[i];
		if (std::holds_alternative<ExprPtr>(v)) {
		    const auto &e = std::get<ExprPtr>(v);
		    if (e) {
			e->print(indent + 4);
		    } else {
			std::printf("%*sZER0\n", indent + 4, "");
		    }
		} else if (std::holds_alternative<ConstExpr>(v)) {
		    std::get<ConstExpr>(v).print(indent + 4);
		}
	    }
	}

    private:
	const Type *type;
	std::size_t pos;
	std::vector<std::variant<ExprPtr, ConstExpr>> value;
	std::vector<const Type *> valueType;
	std::vector<Token::Loc> valueLoc;
};


#endif // CONSTEXPR_HPP
