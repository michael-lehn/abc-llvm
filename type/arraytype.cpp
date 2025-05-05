#include <set>
#include <sstream>
#include <string>

#include "arraytype.hpp"

namespace abc {

static std::string getArrayDimAndType(const Type *refType, std::size_t dim);

static bool
operator<(const ArrayType &x, const ArrayType &y)
{
    const auto &tx = std::tuple{x.ustr().c_str(),
				x.refType(),
				x.dim(),
				x.hasConstFlag(),
				x.hasVolatileFlag()};
    const auto &ty = std::tuple{y.ustr().c_str(),
				y.refType(),
				x.dim(),
				y.hasConstFlag(),
				y.hasVolatileFlag()};
    return tx < ty;
}

static std::set<ArrayType> arraySet;

//------------------------------------------------------------------------------
ArrayType::ArrayType(const Type *refType, std::size_t dim, bool constFlag,
		     bool volatileFlag, UStr name)
    : Type{constFlag, volatileFlag, name}, refType_{refType}, dim_{dim}
{
}

const Type *
ArrayType::create(const Type *refType, std::size_t dim, bool constFlag,
		  bool volatileFlag)
{
    std::stringstream ss;
    ss << "array " << getArrayDimAndType(refType, dim);
    auto ty = ArrayType{refType, dim, constFlag, volatileFlag,
			UStr::create(ss.str())};
    return &*arraySet.insert(ty).first;
}

void 
ArrayType::init()
{
    arraySet.clear();
}

const Type *
ArrayType::create(const Type *refType, std::size_t dim)
{
    return create(refType, dim, false, false);
}

const Type *
ArrayType::getConst() const
{
    return create(refType(), dim(), true, hasVolatileFlag());
}

const Type *
ArrayType::getVolatile() const
{
    return create(refType(), dim(), hasConstFlag(), true);
}

const Type *
ArrayType::getConstRemoved() const
{
    return create(refType(), dim(), false, false);
}

bool
ArrayType::isArray() const
{
    return true;
}

const Type *
ArrayType::refType() const
{
    auto type = refType_;
    if (hasConstFlag()) {
	type = type->getConst();
    }
    if (hasVolatileFlag()) {
	type = type->getVolatile();
    }
    return type;
}

std::size_t
ArrayType::dim() const
{
    return dim_;
}

//------------------------------------------------------------------------------

/*
 * Auxiliary functions
 */

static std::string
getArrayDimAndType(const Type *refType, std::size_t dim)
{
    std::stringstream ss;
    ss << "[";
    if (dim) {
	ss << dim;
    }
    ss << "]";
    if (refType->isArray()) {
	ss << getArrayDimAndType(refType->refType(), refType->dim());
    } else {
	ss << " of " << refType;
    }
    return ss.str();
}

} // namespace abc
