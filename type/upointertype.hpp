#ifndef TYPE_UPOINTERTYPE_HPP
#define TYPE_UPOINTERTYPE_HPP

#include "type.hpp"

namespace abc {

class UPointerType : public Type
{
    private:
	UPointerType(const Type *refType, UStr destructorName, bool constFlag,
		     UStr name);
	const Type *refType_;
	UStr destructorName_;

	static const Type *create(const Type *refType, UStr destructorName,
				  bool constFlag);

    public:
	static void init();
	static const Type *create(const Type *refType, UStr destructorName);
	static const Type *destructorType(const Type *refType);

	const Type *getConst() const override;
	const Type *getConstRemoved() const override;

	bool isPointer() const override;
	bool isUPointer() const override;
	const Type *refType() const override;
	const UStr destructorName() const override;
};

} // namespace abc

#endif // TYPE_POINTERTYPE_HPP
