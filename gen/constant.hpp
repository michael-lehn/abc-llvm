#ifndef GEN_CONSTANT_HPP
#define GEN_CONSTANT_HPP

#include "gen.hpp"
#include "type/type.hpp"

namespace gen {

ConstantInt getConstantInt(const char *val, const abc::Type *type,
                           std::uint8_t radix = 10);
ConstantInt getConstantInt(std::uint64_t val, const abc::Type *type);

ConstantFloat getConstantFloat(const char *val, const abc::Type *type);
ConstantFloat getConstantFloat(double val, const abc::Type *type);

Constant getConstantArray(const std::vector<Constant> &val,
                          const abc::Type *arrayType);
Constant getConstantStruct(const std::vector<Constant> &val,
                           const abc::Type *structType);

Constant getConstantZero(const abc::Type *type);
Constant getFalse();
Constant getTrue();

Constant getString(const char *str);

} // namespace gen

#endif // GEN_CONSTANT_HPP
