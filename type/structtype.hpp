#ifndef TYPE_STRUCTTYPE_HPP
#define TYPE_STRUCTTYPE_HPP

#include "type.hpp"

namespace abc {

class StructType : public Type
{
    protected:
	StructType(std::size_t id, UStr name, bool constFlag);
	std::size_t id_;

	bool isComplete_;
	std::vector<UStr> memberName_;
	std::vector<std::size_t> memberIndex_;
	std::vector<const Type *> memberType_;

    public:
	static void init();
	static Type *createIncomplete(UStr name);

	std::size_t id() const override;

	const Type *getConst() const override;
	const Type *getConstRemoved() const override;

	bool hasSize() const override;

	bool isStruct() const override;
	const Type *complete(std::vector<UStr> &&memberName,
			     std::vector<std::size_t> &&memberIndex,
			     std::vector<const Type *> &&memberType) override;
	const std::vector<UStr> &memberName() const override;
	const std::vector<std::size_t> &memberIndex() const override;
	std::optional<std::size_t> memberIndex(UStr name) const override;
	const std::vector<const Type *> &memberType() const override;
	const Type *memberType(UStr name) const override;
	std::size_t aggregateSize() const override;
	const Type *aggregateType(std::size_t index) const override;
};

} // namespace abc

#endif // TYPE_STRUCTTYPE_HPP
