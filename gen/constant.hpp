#ifndef GEN_CONSTANT_HPP
#define GEN_CONSTANT_HPP

#include "gen.hpp"
#include "type/type.hpp"

namespace gen {

ConstantInt getConstantInt(const char *val, const abc::Type *type,
			   std::uint8_t radix = 10);

} // namespace gen

#endif // GEN_CONSTANT_HPP
