#include <utility>
#include <vector>

#ifdef SUPPORT_SOLARIS
// has to be included as first llvm header
#include "llvm/Support/Solaris/sys/regset.h"
#endif // SUPPORT_SOLARIS

#include "llvm/IR/ValueHandle.h"

#include "analysis.hpp"
#include "nullderef.hpp"
#include "sourceinfo.hpp"
#include "uninit.hpp"

namespace abc {
namespace analysis {

namespace opt {

bool uninitialized = false;
bool nullDereference = false;

} // namespace opt

static bool
enabled()
{
    return opt::uninitialized || opt::nullDereference;
}

static std::vector<Local> local;

// Weak handles as instructions in unreachable blocks get deleted. Not a
// ValueMap: LLVM is usually built without RTTI, and a ValueMap would need the
// typeinfo of its base class CallbackVH.
static std::vector<std::pair<llvm::WeakVH, lexer::Loc>> recordedRead;

void
declareLocal(gen::Value addr, UStr name, lexer::Loc loc)
{
    if (enabled()) {
	local.push_back({llvm::cast<llvm::AllocaInst>(addr), name, loc});
    }
}

void
noteRead(gen::Value load, lexer::Loc loc)
{
    if (enabled()) {
	recordedRead.emplace_back(load, loc);
    }
}

void
run(llvm::Function &fn)
{
    if (!enabled()) {
	return;
    }
    llvm::DenseMap<const llvm::Value *, lexer::Loc> readLoc;
    for (const auto &[load, loc] : recordedRead) {
	if (load) {
	    readLoc[load] = loc;
	}
    }
    SourceInfo info{local, readLoc};
    if (opt::uninitialized) {
	checkUninitialized(fn, info);
    }
    if (opt::nullDereference) {
	checkNullDereference(fn, info);
    }
    local.clear();
    recordedRead.clear();
}

} // namespace analysis
} // namespace abc
