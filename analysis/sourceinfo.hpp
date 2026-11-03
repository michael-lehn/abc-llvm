#ifndef ANALYSIS_SOURCEINFO_HPP
#define ANALYSIS_SOURCEINFO_HPP

#include <string>
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
	// source text of reads of members, e.g. list->next
	const llvm::DenseMap<const llvm::Value *, std::string> &readText;
};

} // namespace analysis
} // namespace abc

#endif // ANALYSIS_SOURCEINFO_HPP
