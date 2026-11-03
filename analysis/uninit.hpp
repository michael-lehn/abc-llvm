#ifndef ANALYSIS_UNINIT_HPP
#define ANALYSIS_UNINIT_HPP

#include "gen/gen.hpp"
#include "lexer/loc.hpp"
#include "util/ustr.hpp"

namespace abc {
namespace analysis {

// All static analyses are off by default. Enabled with -W<name> or -Wall
namespace opt {

extern bool uninitialized;

} // namespace opt

// Source information that LLVM IR does not carry. It gets recorded during code
// generation and is valid until checkUninitialized() is called for the function
void declareLocal(gen::Value addr, UStr name, lexer::Loc loc);
void noteRead(gen::Value load, lexer::Loc loc);

// Forward dataflow analysis on the unoptimized IR of a completed function
void checkUninitialized(llvm::Function &fn);

} // namespace analysis
} // namespace abc

#endif // ANALYSIS_UNINIT_HPP
