#ifndef TYPE_NULLPTR_HPP
#define TYPE_NULLPTR_HPP

#include "type.hpp"

namespace abc {

class NullptrType : public Type
{
    protected:
	NullptrType(bool constFlag, UStr name);

	static const Type *create(bool constFlag, UStr name);

    public:
	static void init();
	static const Type *create();

	const Type *getConst() const override;
	const Type *getConstRemoved() const override;

	bool hasSize() const override;

	bool isNullptr() const override;
	bool isPointer() const override;
	const Type *refType() const override;
};

} // namespace abc

#endif // TYPE_NULLPTR_HPP
