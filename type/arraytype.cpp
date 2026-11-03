#include <set>
#include <sstream>
#include <string>

#include "arraytype.hpp"

namespace abc {

static std::string getArrayDimAndType(const Type *refType, std::size_t dim);

bool
operator<(const ArrayType &x, const ArrayType &y)
{
    // refType_ and not refType(): for a readonly array that creates a type, and
    // so would insert into the set that is being searched
    const auto &tx =
        std::tuple{x.ustr().c_str(), x.refType_, x.dim_, x.hasConstFlag()};
    const auto &ty =
        std::tuple{y.ustr().c_str(), y.refType_, y.dim_, y.hasConstFlag()};
    return tx < ty;
}

static std::set<ArrayType> arraySet;

//------------------------------------------------------------------------------
ArrayType::ArrayType(const Type *refType, std::size_t dim, bool constFlag,
                     UStr name)
    : Type{constFlag, name}, refType_{refType}, dim_{dim}
{
}

const Type *
ArrayType::create(const Type *refType, std::size_t dim, bool constFlag)
{
    std::stringstream ss;
    ss << "array " << getArrayDimAndType(refType, dim);
    auto ty = ArrayType{refType, dim, constFlag, UStr::create(ss.str())};
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
    return create(refType, dim, false);
}

const Type *
ArrayType::getConst() const
{
    return create(refType(), dim(), true);
}

const Type *
ArrayType::getConstRemoved() const
{
    return create(refType(), dim(), false);
}

bool
ArrayType::isArray() const
{
    return true;
}

const Type *
ArrayType::refType() const
{
    if (hasConstFlag()) {
	return refType_->getConst();
    } else {
	return refType_;
    }
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
