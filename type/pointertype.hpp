#ifndef TYPE_POINTERTYPE_HPP
#define TYPE_POINTERTYPE_HPP

#include "type.hpp"

namespace abc {

class PointerType : public Type
{
    private:
	PointerType(const Type *refType, bool constFlag, UStr name);
	const Type *refType_;

	static const Type *create(const Type *refType, bool constFlag);

    public:
	static const Type *create(const Type *refType);

	const Type *getConst() const override;
	const Type *getConstRemoved() const override;

	bool isPointer() const override;
	const Type *refType() const override;
};

} // namespace abc

#endif // TYPE_POINTERTYPE_HPP
