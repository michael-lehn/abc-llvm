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
	UStr name;

    public:
	Type(bool isConst, UStr name);
	virtual ~Type() = default;

	// for type conversion
	static bool equals(const Type *ty1, const Type *ty2);
	static const Type *common(const Type *ty1, const Type *ty2);
	static const Type *convert(const Type *from, const Type *to);
	static const Type *explicitCast(const Type *from, const Type *to);

	virtual const Type *getConst() const = 0;
	virtual const Type *getConstRemoved() const = 0;

	UStr ustr() const;
	virtual std::size_t id() const;
	bool hasConstFlag() const;
	bool isScalar() const;
	std::size_t aggregateSize() const;
	const Type *aggregateType(std::size_t index) const;

	// for type aliases
	virtual bool isAlias() const;
	virtual const Type *getUnalias() const;
	const Type *getAlias(const char *alias) const;
	const Type *getAlias(UStr alias) const;
	// ---

	virtual bool hasSize() const;

	virtual bool isVoid() const;
	virtual bool isNullptr() const;


	// for integer (sub-)types 
	virtual bool isBool() const;
	virtual bool isInteger() const;
	virtual bool isSignedInteger() const;
	virtual bool isUnsignedInteger() const;
	virtual std::size_t numBits() const;

	// for floating point type (sub-)types 
	virtual bool isFloatType() const;
	virtual bool isFloat() const;
	virtual bool isDouble() const;

	// for pointer and array (sub-)types
	virtual bool isPointer() const;
	virtual bool isUPointer() const;
	virtual bool isArray() const;
	bool isUnboundArray() const;
	virtual const Type *refType() const;
	virtual const UStr destructorName() const;
	virtual std::size_t dim() const;
	static const Type *patchUnboundArray(const Type *type, std::size_t dim);

	// for function (sub-)types 
	virtual bool isFunction() const;
	virtual const Type *retType() const;
	virtual const std::vector<const Type *> &paramType() const;
	virtual bool hasVarg() const;

	// for enum (sub-)types
	virtual bool isEnum() const;
	virtual const Type *complete(std::vector<UStr> &&constName,
				     std::vector<std::int64_t> &&constValue);

	// for struct (sub-)types
	virtual bool isStruct() const;
	virtual const Type *complete(std::vector<UStr> &&memberName,
				     std::vector<std::size_t> &&memberIndex,
				     std::vector<const Type *> &&memberType);
	virtual const std::vector<UStr> &memberName() const;
	virtual const std::vector<std::size_t> &memberIndex() const;
	virtual const std::vector<const Type *> &memberType() const;

};

std::ostream &operator<<(std::ostream &out, const Type *type);

} // namespace abc

#endif // TYPE_TYPE_HPP
