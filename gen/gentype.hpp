#ifndef GEN_GENTYPE_HPP
#define GEN_GENTYPE_HPP

#include "llvm/IR/Type.h"

#include "type/type.hpp"

namespace gen {

llvm::Type *convert(const abc::Type *type);
std::size_t getSizeof(const abc::Type *type);


} // namespace gen

#endif // GEN_GENTYPE_HPP
