#ifndef TYPE_ENUMTYPE_HPP
#define TYPE_ENUMTYPE_HPP

#include "type.hpp"

namespace abc {

class EnumType : public Type
{
    protected:
	EnumType(std::size_t id, UStr name, const Type *intType,
		 bool constFlag);
	std::size_t id;
	const Type *intType;

	bool isComplete_;
	std::vector<UStr> constName;
	std::vector<std::int64_t> constValue;

    public:
	static Type *createIncomplete(UStr name, const Type *intType);

	const Type *getConst() const override;
	const Type *getConstRemoved() const override;

	bool hasSize() const override;
	std::size_t numBits() const override;

	bool isInteger() const override;
	bool isSignedInteger() const override;
	bool isUnsignedInteger() const override;

	bool isEnum() const override;
	const Type *complete(
			const std::vector<UStr> &&constName,
			const std::vector<std::int64_t> &&constValue) override;
};

} // namespace abc

#endif // TYPE_ENUMTYPE_HPP
