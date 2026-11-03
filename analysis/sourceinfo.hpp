#ifndef ANALYSIS_SOURCEINFO_HPP
#define ANALYSIS_SOURCEINFO_HPP

#include <vector>

#ifdef SUPPORT_SOLARIS
// has to be included as first llvm header
#include "llvm/Support/Solaris/sys/regset.h"
#endif // SUPPORT_SOLARIS

#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Instructions.h"

#include "lexer/loc.hpp"
#include "util/ustr.hpp"

namespace abc {
namespace analysis {

struct Local
{
	llvm::AllocaInst *addr;
	UStr name;
	lexer::Loc loc;
};

// What an analysis gets to know about a function besides its IR
struct SourceInfo
{
	const std::vector<Local> &local;
	const llvm::DenseMap<const llvm::Value *, lexer::Loc> &readLoc;
};

} // namespace analysis
} // namespace abc

#endif // ANALYSIS_SOURCEINFO_HPP
