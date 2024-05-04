#ifndef TYPE_FLOATTYPE_HPP
#define TYPE_FLOATTYPE_HPP

#include "type.hpp"

namespace abc {

class FloatType : public Type
{
    protected:
	enum FloatKind {
	    FLOAT_KIND,
	    DOUBLE_KIND
	};

	FloatType(FloatKind floatKind, bool constFlag, UStr name);
	const FloatKind floatKind;

	static const Type *create(FloatKind floatKind, bool constFlag);

    public:
	static void init();
	static const Type *createFloat();
	static const Type *createDouble();

	const Type *getConst() const override;
	const Type *getConstRemoved() const override;

	bool isFloatType() const override;
	bool isFloat() const override;
	bool isDouble() const override;

	friend bool
	    operator<(const FloatType &x, const FloatType &y);
};

} // namespace abc

#endif // TYPE_FLOATTYPE_HPP
