#include <string>
#include <utility>
#include <vector>

#ifdef SUPPORT_SOLARIS
// has to be included as first llvm header
#include "llvm/Support/Solaris/sys/regset.h"
#endif // SUPPORT_SOLARIS

#include "llvm/IR/ValueHandle.h"

#include "analysis.hpp"
#include "nullderef-v2.hpp"
#include "nullderef.hpp"
#include "sourceinfo.hpp"
#include "uninit.hpp"

namespace abc {
namespace analysis {

namespace opt {

bool uninitialized = false;
bool nullDereference = false;
bool nullDereferenceStrict = false;

} // namespace opt

bool
recording()
{
    return opt::uninitialized || opt::nullDereference ||
           opt::nullDereferenceStrict;
}

static std::vector<Local> local;

// Weak handles as instructions in unreachable blocks get deleted. Not a
// ValueMap: LLVM is usually built without RTTI, and a ValueMap would need the
// typeinfo of its base class CallbackVH.
static std::vector<std::pair<llvm::WeakVH, lexer::Loc>> recordedRead;
static std::vector<std::pair<llvm::WeakVH, std::string>> recordedText;

void
declareLocal(gen::Value addr, UStr name, lexer::Loc loc)
{
    if (recording()) {
	local.push_back({llvm::cast<llvm::AllocaInst>(addr), name, loc});
    }
}

void
noteRead(gen::Value load, lexer::Loc loc)
{
    if (recording()) {
	recordedRead.emplace_back(load, loc);
    }
}

void
noteMemberRead(gen::Value load, lexer::Loc loc, const std::string &text)
{
    if (recording()) {
	recordedRead.emplace_back(load, loc);
	recordedText.emplace_back(load, text);
    }
}

void
run(llvm::Function &fn)
{
    if (!recording()) {
	return;
    }
    llvm::DenseMap<const llvm::Value *, lexer::Loc> readLoc;
    for (const auto &[load, loc] : recordedRead) {
	if (load) {
	    readLoc[load] = loc;
	}
    }
    llvm::DenseMap<const llvm::Value *, std::string> readText;
    for (const auto &[load, text] : recordedText) {
	if (load) {
	    readText[load] = text;
	}
    }
    SourceInfo info{local, readLoc, readText};
    if (opt::uninitialized) {
	checkUninitialized(fn, info);
    }
    if (opt::nullDereferenceStrict) {
	checkNullDereferenceStrict(fn, info);
    } else if (opt::nullDereference) {
	checkNullDereference(fn, info);
    }
    local.clear();
    recordedRead.clear();
    recordedText.clear();
}

} // namespace analysis
} // namespace abc
