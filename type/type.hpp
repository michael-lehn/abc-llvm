#ifndef TYPE_TYPE_HPP
#define TYPE_TYPE_HPP

#include <cstddef>
#include <ostream>
#include <vector>

#include "lexer/ustr.hpp"

namespace type {

class Type
{
    protected:
	UStr name, alias;

    public:
	Type(UStr alias);
	virtual ~Type() = default;

	const Type *getAlias(const char *alias) const;
	virtual const Type *getAlias(UStr alias) const = 0;
	virtual const Type *getConst() const = 0;
	virtual const Type *getConstRemoved() const = 0;

	UStr ustr() const;
	UStr aka() const;

	virtual bool hasSize() const = 0;
	virtual bool hasConstFlag() const = 0;

	// for integer (sub-)types 
	virtual bool isInteger() const;
	virtual bool isSignedInteger() const;
	virtual bool isUnsignedInteger() const;
	virtual std::size_t numBits() const;

	// for pointer and array (sub-)types
	virtual bool isPointer() const;
	virtual const Type *refType() const;
	virtual std::size_t dim() const;

	// for function (sub-)types 
	virtual bool isFunction() const;
	virtual const Type *retType() const;
	virtual bool hasVarg() const;
	virtual const std::vector<const Type *> &argType() const;
};

std::ostream &operator<<(std::ostream &out, const Type *type);

} // namespace type

#endif // TYPE_TYPE_HPP
