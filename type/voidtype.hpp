#ifndef TYPE_VOID_HPP
#define TYPE_VOID_HPP

#include "type.hpp"

namespace abc {

class VoidType : public Type
{
    protected:
	VoidType(bool constFlag, UStr alias);

	static const Type *create(bool constFlag, UStr name);
    public:
	static const Type *create();

	virtual const Type *getAlias(UStr alias) const override;
	const Type *getConst() const override;
	const Type *getConstRemoved() const override;

	bool hasSize() const override;
	bool hasConstFlag() const override;

	virtual bool isVoid() const override;
};

} // namespace abc

#endif // TYPE_VOID_HPP
