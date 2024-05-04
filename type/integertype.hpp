#ifndef TYPE_INTEGERTYPE_HPP
#define TYPE_INTEGERTYPE_HPP

#include "type.hpp"

namespace abc {

class IntegerType : public Type
{
    protected:
	IntegerType(std::size_t numBits, bool signed_, bool constFlag,
		    UStr name);
	std::size_t numBits_;
	bool isSigned;

	static const Type *create(std::size_t numBits, bool signed_,
				  bool constFlag);

    public:
	static void init();
	static const Type *createBool();
	static const Type *createChar();
	static const Type *createInt();
	static const Type *createUnsigned();
	static const Type *createLong();
	static const Type *createSizeType();
	static const Type *createPtrdiffType();
	static const Type *createSigned(std::size_t numBits);
	static const Type *createUnsigned(std::size_t numBits);

	const Type *getConst() const override;
	const Type *getConstRemoved() const override;

	std::size_t numBits() const override;

	bool isBool() const override;
	bool isInteger() const override;
	bool isSignedInteger() const override;
	bool isUnsignedInteger() const override;
};

} // namespace abc

#endif // TYPE_INTEGERTYPE_HPP
