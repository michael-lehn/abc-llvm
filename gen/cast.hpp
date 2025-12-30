#ifndef GEN_CAST_HPP
#define GEN_CAST_HPP

#include "type/type.hpp"

#include "gen.hpp"

namespace gen {

Value cast(Value val, const abc::Type *fromType, const abc::Type *toType);
Constant cast(Constant val, const abc::Type *fromType, const abc::Type *toType);

} // namespace gen

#endif // GEN_CAST_HPP
