#ifndef FUNCTIONTYPE_HPP
#define FUNCTIONTYPE_HPP

#include <vector>

#include "type.hpp"

namespace type {

class FunctionType : public Type
{
    protected:
	FunctionType(const Type *ret, std::vector<const Type *> &&arg,
		     bool varg, UStr alias);
	const Type *ret;
	std::vector<const Type *> arg;
	bool varg;

	static const Type *create(const Type *ret,
				  std::vector<const Type *> &&arg,
				  bool varg, UStr alias);

    public:
	static const Type *create(const Type *ret,
				  std::vector<const Type *> &&arg,
				  bool varg);

	const Type *getAlias(UStr alias) const override;
	const Type *getConst() const override;
	const Type *getConstRemoved() const override;

	bool hasSize() const override;
	bool hasConstFlag() const override;

	// for function (sub-)types 
	bool isFunction() const override;
	const Type *retType() const override;
	bool hasVarg() const override;
	const std::vector<const Type *> &argType() const override;
};

} // namespace type

#endif // FUNCTIONTYPE_HPP
