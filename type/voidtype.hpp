#ifndef TYPE_VOID_HPP
#define TYPE_VOID_HPP

#include "type.hpp"

namespace abc {

class VoidType : public Type
{
    protected:
	VoidType(bool constFlag, UStr name);

	static const Type *create(bool constFlag, UStr name);
    public:
	static const Type *create();

	const Type *getConst() const override;
	const Type *getConstRemoved() const override;

	bool hasSize() const override;

	virtual bool isVoid() const override;
};

} // namespace abc

#endif // TYPE_VOID_HPP
