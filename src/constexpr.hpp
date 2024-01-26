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
	ConstExpr(const Type *type)
	    : type{type}, pos{0}, value{type->getNumMembers()}
	{
	}

	const Type *getType() const
	{
	    return type;
	}

	void add(ExprPtr &&expr)
	{
	    assert(expr->isConst());
	    auto ty = type->getMemberType(pos);
	    expr = Expr::createCast(std::move(expr), ty, expr->getLoc());
	    value.at(pos++) = std::move(expr);
	}

	void add(ConstExpr &&constExpr)
	{
	    value.at(pos++) = std::move(constExpr);
	}

	std::size_t size(void) const
	{
	    return value.size();
	}

	gen::ConstVal load(void) const
	{
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
	    auto &v = value.at(index);
	    auto ty = type->getMemberType(index);
	    if (std::holds_alternative<ExprPtr>(v)) {
		const auto &e = std::get<ExprPtr>(v);
		if (e) {
		    return e->loadConst();
		} else {
		    return gen::loadZero(ty);
		}
	    } else if (std::holds_alternative<ConstExpr>(v)) {
		const auto &v = std::get<ConstExpr>(value.at(index));
		return v.load();
	    }
	    assert(0);
	    return nullptr;
	}

	void print(int indent = 0) const
	{
	    for (std::size_t i = 0; i < size(); ++i) {
		const auto &v = value.at(i);
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
};


#endif // CONSTEXPR_HPP
