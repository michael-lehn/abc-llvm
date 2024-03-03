#include <sstream>

#include "functiontype.hpp"

namespace abc {

static bool
operator<(const FunctionType &x, const FunctionType &y)
{
    const auto &tx = std::tuple{x.retType(),
				x.ustr().c_str(),
				x.paramType(),
				x.hasConstFlag()};
    const auto &ty = std::tuple{y.retType(),
				y.ustr().c_str(),
				y.paramType(),
				y.hasConstFlag()};
    return tx < ty;
}

static std::set<FunctionType> fnSet;

//------------------------------------------------------------------------------

FunctionType::FunctionType(const Type *ret, std::vector<const Type *> &&param,
			   bool varg, bool constFlag, UStr name)
    : Type{constFlag, name}, ret{ret}, param{std::move(param)}, varg{varg}
{
}

const Type *
FunctionType::create(const Type *ret, std::vector<const Type *> &&param,
		     bool varg, bool constFlag, UStr alias)
{
    auto ty = FunctionType{ret, std::move(param), varg, constFlag, alias};
    return &*fnSet.insert(ty).first;
}

const Type *
FunctionType::create(const Type *ret, std::vector<const Type *> &&param,
		     bool varg)
{
    std::stringstream ss;
    ss << "fn (";
    for (std::size_t i = 0; i < param.size(); ++i) {
	ss << ":" << param[i];
	if (i + 1 < param.size()) {
	    ss << ", ";
	}
    }
    ss << "): " << ret;
    return create(ret, std::move(param), varg, false, UStr::create(ss.str()));
}


const Type *
FunctionType::getConst() const
{
    std::vector<const Type *> paramTy = paramType();
    return create(retType(), std::move(paramTy), hasVarg(), true, name);
}

const Type *
FunctionType::getConstRemoved() const
{
    std::vector<const Type *> paramTy = paramType();
    return create(retType(), std::move(paramTy), hasVarg(), false, name);
}

bool
FunctionType::hasSize() const
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
FunctionType::paramType() const
{
    return param;
}

} // namespace abc
