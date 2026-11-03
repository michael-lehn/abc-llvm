#ifndef ANALYSIS_UNINIT_HPP
#define ANALYSIS_UNINIT_HPP

#include "gen/gen.hpp"

#include "sourceinfo.hpp"

namespace abc {
namespace analysis {

// Forward dataflow analysis: local variables that are or may be used
// uninitialized
void checkUninitialized(llvm::Function &fn, const SourceInfo &info);

} // namespace analysis
} // namespace abc

#endif // ANALYSIS_UNINIT_HPP
