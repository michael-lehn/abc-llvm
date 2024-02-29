#ifndef TYPE_TYPE_HPP
#define TYPE_TYPE_HPP

#include <cstddef>
#include <ostream>
#include <vector>

#include "util/ustr.hpp"

namespace abc {

class Type
{
    protected:
	bool isConst;
	UStr name, aka_;

    public:
	Type(bool isConst, UStr name);
	virtual ~Type() = default;

	const Type *getAlias(const char *alias) const;
	virtual const Type *getAlias(UStr alias) const = 0;
	virtual const Type *getConst() const = 0;
	virtual const Type *getConstRemoved() const = 0;

	UStr ustr() const;
	UStr aka() const;

	virtual bool hasSize() const = 0;
	virtual bool hasConstFlag() const;

	virtual bool isVoid() const;
	virtual bool isBool() const;
	virtual bool isNullptr() const;

	// for integer (sub-)types 
	virtual bool isInteger() const;
	virtual bool isSignedInteger() const;
	virtual bool isUnsignedInteger() const;
	virtual std::size_t numBits() const;

	// for pointer and array (sub-)types
	virtual bool isPointer() const;
	virtual bool isArray() const;
	virtual const Type *refType() const;
	virtual std::size_t dim() const;

	// for function (sub-)types 
	virtual bool isFunction() const;
	virtual const Type *retType() const;
	virtual const std::vector<const Type *> &paramType() const;
	virtual bool hasVarg() const;

	// for struct (sub-)types
	virtual bool isStruct() const;
	virtual const Type *complete(
		const std::vector<UStr> &&memberIdent,
		const std::vector<const Type *> &&memberType);
	const std::vector<const Type *> &memberType() const;
	const std::vector<UStr> &memberIdent() const;
};

std::ostream &operator<<(std::ostream &out, const Type *type);

} // namespace abc

#endif // TYPE_TYPE_HPP
