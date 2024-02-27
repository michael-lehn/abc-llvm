#ifndef INTEGERTYPE_HPP
#define INTEGERTYPE_HPP

#include "type.hpp"

namespace type {

class IntegerType : public Type
{
    protected:
	IntegerType(std::size_t numBits, bool signed_, bool constFlag,
		    UStr alias);
	std::size_t numBits_;
	bool isSigned;
	bool isConst;

	static const Type *create(std::size_t numBits, bool signed_,
				  bool constFlag, UStr alias);

    public:
	static const Type *createSigned(std::size_t numBits);
	static const Type *createUnsigned(std::size_t numBits);

	virtual const Type *getAlias(UStr alias) const override;
	const Type *getConst() const override;
	const Type *getConstRemoved() const override;

	bool hasSize() const override;
	bool hasConstFlag() const override;
	std::size_t numBits() const override;

	bool isInteger() const override;
	bool isSignedInteger() const override;
	bool isUnsignedInteger() const override;
};

} // namespace type

#endif // INTEGERTYPE_HPP
