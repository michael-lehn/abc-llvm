#include <sstream>

#include "functiontype.hpp"

namespace type {

static bool
operator<(const FunctionType &x, const FunctionType &y)
{
    const auto &tx = std::tuple{x.retType(),
				x.ustr().c_str(),
				x.aka().c_str(),
				x.argType()};
    const auto &ty = std::tuple{y.retType(),
				y.ustr().c_str(),
				y.aka().c_str(),
				y.argType()};
    return tx < ty;
}

static std::set<FunctionType> fnSet;

//------------------------------------------------------------------------------

FunctionType::FunctionType(const Type *ret, std::vector<const Type *> &&arg,
			   bool varg, UStr alias)
    : Type{alias}, ret{ret}, arg{std::move(arg)}, varg{varg}
{
    std::stringstream ss;
    ss << "fn (";
    for (std::size_t i = 0; i < this->arg.size(); ++i) {
	ss << ":" << this->arg[i]->aka();
	if (i + 1 < this->arg.size()) {
	    ss << ", ";
	}
    }
    ss << "): " << ret->aka();
    name = UStr::create(ss.str());
}

const Type *
FunctionType::create(const Type *ret, std::vector<const Type *> &&arg,
		     bool varg, UStr alias)
{
    auto ty = FunctionType{ret, std::move(arg), varg, alias};
    return &*fnSet.insert(ty).first;
}

const Type *
FunctionType::create(const Type *ret, std::vector<const Type *> &&arg,
		     bool varg)
{
    return create(ret, std::move(arg), varg, UStr{});
}


const Type *
FunctionType::getAlias(UStr alias) const
{
    std::vector<const Type *> argTy = argType();
    return create(retType(), std::move(argTy), hasVarg(), alias);
}

const Type *
FunctionType::getConst() const
{
    return this;
}

const Type *
FunctionType::getConstRemoved() const
{
    return this;
}

bool
FunctionType::hasSize() const
{
    return false;
}

bool
FunctionType::hasConstFlag() const
{
    return false;
}

// for function (sub-)types 
bool
FunctionType::isFunction() const
{
    return true;
}

const Type *
FunctionType::retType() const
{
    return ret;
}

bool
FunctionType::hasVarg() const
{
    return varg;
}

const std::vector<const Type *> &
FunctionType::argType() const
{
    return arg;
}

} // namespace type
