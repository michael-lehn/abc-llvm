#ifndef TYPE_AUTO_HPP
#define TYPE_AUTO_HPP

#include "type.hpp"

namespace abc {

class AutoType : public Type
{
    protected:
	AutoType(bool constFlag, bool volatileFlag, UStr name);

	static const Type *create(bool constFlag, bool volatileFlag,
				  UStr name);

    public:
	static void init();
	static const Type *create();

	const Type *getConst() const override;
	const Type *getVolatile() const override;
	const Type *getConstRemoved() const override;

	bool hasSize() const override;

	virtual bool isAuto() const override;
};

} // namespace abc

#endif // TYPE_AUTO_HPP
