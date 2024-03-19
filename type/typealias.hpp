#ifndef TYPE_ALIAS_HPP
#define TYPE_ALIAS_HPP

#include <cstddef>
#include <ostream>
#include <vector>

#include "util/ustr.hpp"

#include "type.hpp"

namespace abc {

class TypeAlias : public Type
{
    private:
	TypeAlias(std::size_t id, UStr name, const Type *type, bool constFlag);
	std::size_t id;
	const Type *type;

    public:
	static void init();
	static const Type *create(UStr name, const Type *type);

	bool isAlias() const override;
	const Type *getUnalias() const override;

	const Type *getConst() const override;
	const Type *getConstRemoved() const override;
};

} // namespace abc

#endif // TYPE_ALIAS_HPP
