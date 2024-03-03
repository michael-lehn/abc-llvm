#ifndef TYPE_FUNCTIONTYPE_HPP
#define TYPE_FUNCTIONTYPE_HPP

#include <vector>

#include "type.hpp"

namespace abc {

class FunctionType : public Type
{
    protected:
	FunctionType(const Type *ret, std::vector<const Type *> &&param,
		     bool varg, bool constFlag, UStr name);
	const Type *ret;
	std::vector<const Type *> param;
	bool varg;

	static const Type *create(const Type *ret,
				  std::vector<const Type *> &&arg,
				  bool varg, bool constFlag, UStr alias);

    public:
	static const Type *create(const Type *ret,
				  std::vector<const Type *> &&arg,
				  bool varg = false);

	const Type *getConst() const override;
	const Type *getConstRemoved() const override;

	bool hasSize() const override;

	// for function (sub-)types 
	bool isFunction() const override;
	const Type *retType() const override;
	bool hasVarg() const override;
	const std::vector<const Type *> &paramType() const override;
};

} // namespace abc

#endif // TYPE_FUNCTIONTYPE_HPP
