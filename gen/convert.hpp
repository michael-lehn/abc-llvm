#ifndef GEN_CONVERT_HPP
#define GEN_CONVERT_HPP

#include "llvm/IR/Type.h"

#include "type/type.hpp"

namespace gen {

llvm::Type *convert(const abc::Type *type);

} // namespace gen

#endif // GEN_CONVERT_HPP
