#ifndef TYPE_ARRAYTYPE_HPP
#define TYPE_ARRAYTYPE_HPP

#include "type.hpp"

namespace abc {

class ArrayType : public Type
{
    private:
	ArrayType(const Type *refType, std::size_t dim, bool constFlag,
		  bool volatileFlag, UStr name);
	const Type *refType_;
	const std::size_t dim_;

	static const Type *create(const Type *refType, std::size_t dim,
				  bool constFlag, bool volatileFlag);

    public:
	static void init();
	static const Type *create(const Type *refType, std::size_t dim);

	const Type *getConst() const override;
	const Type *getVolatile() const override;
	const Type *getConstRemoved() const override;

	bool isArray() const override;
	const Type *refType() const override;
	std::size_t dim() const override;
};

} // namespace abc

#endif // TYPE_ARRAYTYPE_HPP
