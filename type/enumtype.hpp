#ifndef TYPE_ENUMTYPE_HPP
#define TYPE_ENUMTYPE_HPP

#include "type.hpp"

namespace abc {

class EnumType : public Type
{
    protected:
	EnumType(std::size_t id, UStr name, Type *intType, bool constFlag);
	std::size_t id;
	Type *intType;
	bool isComplete_;

    public:
	static const Type *create(UStr name, Type *intType);

	const Type *getConst() const override;
	const Type *getConstRemoved() const override;

	bool isComplete() const;

	bool hasSize() const override;
	std::size_t numBits() const override;

	bool isInteger() const override;
	bool isSignedInteger() const override;
	bool isUnsignedInteger() const override;
};

} // namespace abc

#endif // TYPE_ENUMTYPE_HPP
