#ifndef ANALYSIS_ANALYSIS_HPP
#define ANALYSIS_ANALYSIS_HPP

#include "gen/gen.hpp"
#include "lexer/loc.hpp"
#include "util/ustr.hpp"

namespace abc {
namespace analysis {

// All static analyses are off by default. Enabled with -W<name> or -Wall
namespace opt {

extern bool uninitialized;
extern bool nullDereference;

} // namespace opt

// Source information that LLVM IR does not carry. It gets recorded during code
// generation if an analysis is enabled, and is valid until run() is called for
// the function
void declareLocal(gen::Value addr, UStr name, lexer::Loc loc);
void noteRead(gen::Value load, lexer::Loc loc);

// Runs the enabled analyses on the unoptimized IR of a completed function
void run(llvm::Function &fn);

} // namespace analysis
} // namespace abc

#endif // ANALYSIS_ANALYSIS_HPP
