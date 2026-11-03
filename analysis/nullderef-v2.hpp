#ifndef ANALYSIS_NULLDEREF_V2_HPP
#define ANALYSIS_NULLDEREF_V2_HPP

#include "gen/gen.hpp"

#include "sourceinfo.hpp"

namespace abc {
namespace analysis {

// Forward dataflow analysis like checkNullDereference() that also tracks members
// of structs and tests kept in a variable. A member nothing is known about may
// be null.
void checkNullDereferenceStrict(llvm::Function &fn, const SourceInfo &info);

} // namespace analysis
} // namespace abc

#endif // ANALYSIS_NULLDEREF_V2_HPP
