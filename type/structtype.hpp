#ifndef TYPE_STRUCTTYPE_HPP
#define TYPE_STRUCTTYPE_HPP

#include "type.hpp"

namespace abc {

class StructType : public Type
{
    protected:
	StructType(std::size_t id, UStr name, bool constFlag);
	std::size_t id;

	bool isComplete_;
	std::vector<UStr> memberName_;
	std::vector<const Type *> memberType_;

    public:
	static Type *createIncomplete(UStr name);

	const Type *getConst() const override;
	const Type *getConstRemoved() const override;

	bool hasSize() const override;

	bool isStruct() const override;
	const Type *complete(std::vector<UStr> &&memberName,
			     std::vector<const Type *> &&memberType) override;
	const std::vector<UStr> &memberName() const override;
	const std::vector<const Type *> &memberType() const override;
};

} // namespace abc

#endif // TYPE_STRUCTTYPE_HPP
