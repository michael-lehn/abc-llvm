#ifndef ANALYSIS_NULLDEREF_HPP
#define ANALYSIS_NULLDEREF_HPP

#include "gen/gen.hpp"

#include "sourceinfo.hpp"

namespace abc {
namespace analysis {

// Forward dataflow analysis: local pointer variables that are or may be null or
// freed when dereferenced, and pointers that are or may be freed twice
void checkNullDereference(llvm::Function &fn, const SourceInfo &info);

} // namespace analysis
} // namespace abc

#endif // ANALYSIS_NULLDEREF_HPP
