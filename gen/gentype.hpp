#ifndef GEN_GENTYPE_HPP
#define GEN_GENTYPE_HPP

#ifdef SUPPORT_SOLARIS
// has to be included as first llvm header
#include "llvm/Support/Solaris/sys/regset.h"
#endif // SUPPORT_SOLARIS

#include "llvm/IR/Type.h"

#include "type/type.hpp"

namespace gen {

void initTypeMap();
llvm::Type *convert(const abc::Type *type);
std::size_t getSizeof(const abc::Type *type);

} // namespace gen

#endif // GEN_GENTYPE_HPP
