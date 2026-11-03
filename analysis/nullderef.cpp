#include <algorithm>
#include <cstdint>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#ifdef SUPPORT_SOLARIS
// has to be included as first llvm header
#include "llvm/Support/Solaris/sys/regset.h"
#endif // SUPPORT_SOLARIS

#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"

#include "lexer/error.hpp"

#include "nullderef.hpp"

namespace abc {
namespace analysis {

namespace {

// Set of values a pointer variable can have on the paths reaching a program
// point. A value returned by malloc is {IS_NULL, NON_NULL}. UNKNOWN is a value
// nothing is known about, e.g. a result of a function: it gets no warning, but a
// test tells which value it has on each edge.
using State = std::uint8_t;
constexpr State IS_NULL = 1, NON_NULL = 2, FREED = 4, UNKNOWN = 8;
using StateVec = std::vector<State>;

enum class ReportKind
{
    NULL_DEREF,
    FREED_DEREF,
    DOUBLE_FREE
};

struct Report
{
	const llvm::Instruction *read;
	std::size_t var;
	State state;
	ReportKind kind;
};

using Index = llvm::DenseMap<const llvm::Value *, std::size_t>;

struct BlockResult
{
	// variable whose value a pointer is based on, and the load of it
	llvm::DenseMap<const llvm::Value *,
	               std::pair<std::size_t, const llvm::Value *>>
	    base;
	// variable holding this value (for tests against null)
	llvm::DenseMap<const llvm::Value *, std::size_t> holds;
	bool noReturn = false;
};

} // namespace

static bool
calledName(const llvm::CallInst *call, std::initializer_list<const char *> name)
{
    auto fn = call->getCalledFunction();
    if (!fn) {
	return false;
    }
    return std::any_of(name.begin(), name.end(),
                       [&](const char *n) { return fn->getName() == n; });
}

static State
stateOfValue(const llvm::Value *val)
{
    if (llvm::isa<llvm::ConstantPointerNull>(val)) {
	return IS_NULL;
    }
    if (auto call = llvm::dyn_cast<llvm::CallInst>(val)) {
	if (calledName(call, {"malloc", "calloc", "realloc"})) {
	    return IS_NULL | NON_NULL;
	}
    }
    if (llvm::isa<llvm::AllocaInst>(val) || llvm::isa<llvm::GlobalValue>(val)) {
	return NON_NULL;
    }
    return UNKNOWN;
}

template <typename OnReport>
static BlockResult
transfer(const llvm::BasicBlock &bb, StateVec &state, const Index &index,
         OnReport onReport)
{
    BlockResult r;
    auto deref = [&](const llvm::Value *ptr) {
	auto it = r.base.find(ptr);
	if (it == r.base.end()) {
	    return;
	}
	auto [var, read] = it->second;
	if (state[var] & FREED) {
	    onReport(Report{llvm::cast<llvm::Instruction>(read), var, state[var],
	                    ReportKind::FREED_DEREF});
	} else if (state[var] & IS_NULL) {
	    onReport(Report{llvm::cast<llvm::Instruction>(read), var, state[var],
	                    ReportKind::NULL_DEREF});
	}
	// what follows is only reached if the pointer was valid
	state[var] = NON_NULL;
    };

    for (const auto &inst : bb) {
	if (auto store = llvm::dyn_cast<llvm::StoreInst>(&inst)) {
	    deref(store->getPointerOperand());
	    auto it = index.find(store->getPointerOperand());
	    if (it != index.end()) {
		auto var = it->second;
		state[var] = stateOfValue(store->getValueOperand());
		for (auto h = r.holds.begin(); h != r.holds.end(); ++h) {
		    if (h->second == var) {
			r.holds.erase(h);
		    }
		}
		r.holds[store->getValueOperand()] = var;
	    }
	} else if (auto load = llvm::dyn_cast<llvm::LoadInst>(&inst)) {
	    deref(load->getPointerOperand());
	    auto it = index.find(load->getPointerOperand());
	    if (it != index.end()) {
		r.base[load] = {it->second, load};
		r.holds[load] = it->second;
	    }
	} else if (auto gep = llvm::dyn_cast<llvm::GetElementPtrInst>(&inst)) {
	    auto it = r.base.find(gep->getPointerOperand());
	    if (it != r.base.end()) {
		r.base[gep] = it->second;
	    }
	} else if (auto call = llvm::dyn_cast<llvm::CallInst>(&inst)) {
	    if (calledName(call, {"free"}) && call->arg_size() == 1) {
		auto it = r.base.find(call->getArgOperand(0));
		if (it != r.base.end()) {
		    auto [var, read] = it->second;
		    if (state[var] & FREED) {
			onReport(Report{llvm::cast<llvm::Instruction>(read), var,
			                state[var], ReportKind::DOUBLE_FREE});
		    }
		    state[var] = FREED;
		}
	    }
	    for (unsigned i = 0; i < call->arg_size(); ++i) {
		if (call->paramHasAttr(i, llvm::Attribute::ByVal)) {
		    deref(call->getArgOperand(i));
		}
	    }
	    if (calledName(call, {"exit", "abort", "_Exit", "__assert"})) {
		r.noReturn = true;
		return r;
	    }
	}
    }
    return r;
}

// Applies what the condition of a branch tells about the state of a variable if
// the successor with number succNo is taken. Returns false if this can not
// happen: code generation keeps branches on constants, and a test can contradict
// what is known.
static bool
refine(const llvm::BasicBlock *bb, unsigned succNo, const BlockResult &r,
       StateVec &state)
{
    auto br = bb->getTerminator();
    if (br->getNumSuccessors() != 2 || br->getNumOperands() != 3) {
	return true;
    }
    const llvm::Value *cond = br->getOperand(0);
    if (auto c = llvm::dyn_cast<llvm::ConstantInt>(cond)) {
	return succNo == (c->isZero() ? 1u : 0u);
    }
    bool takenIfTrue = succNo == 0;
    // peel off comparisons of a truth value with false, e.g. 'icmp ne i1 %c, 0'
    for (;;) {
	auto icmp = llvm::dyn_cast<llvm::ICmpInst>(cond);
	if (!icmp) {
	    return true;
	}
	auto lhs = icmp->getOperand(0);
	auto rhs = icmp->getOperand(1);
	if (llvm::isa<llvm::ConstantPointerNull>(lhs)) {
	    std::swap(lhs, rhs);
	}
	bool isNe = icmp->getPredicate() == llvm::ICmpInst::ICMP_NE;
	bool isEq = icmp->getPredicate() == llvm::ICmpInst::ICMP_EQ;
	if (!isNe && !isEq) {
	    return true;
	}
	if (llvm::isa<llvm::ConstantPointerNull>(rhs)) {
	    auto it = r.holds.find(lhs);
	    if (it == r.holds.end()) {
		return true;
	    }
	    bool nonNullEdge = isNe == takenIfTrue;
	    auto &s = state[it->second];
	    if (nonNullEdge) {
		s = (s & (NON_NULL | FREED)) | ((s & UNKNOWN) ? NON_NULL : 0);
	    } else {
		s = (s & (IS_NULL | UNKNOWN)) ? IS_NULL : 0;
	    }
	    return s != 0;
	}
	auto ci = llvm::dyn_cast<llvm::ConstantInt>(rhs);
	if (!ci || !lhs->getType()->isIntegerTy(1)) {
	    return true;
	}
	bool same = isNe == ci->isZero();
	takenIfTrue = takenIfTrue == same;
	cond = lhs;
    }
}

void
checkNullDereference(llvm::Function &fn, const SourceInfo &info)
{
    const auto &readLoc = info.readLoc;

    std::vector<const Local *> tracked;
    Index index;
    for (const auto &var : info.local) {
	if (var.addr->getAllocatedType()->isPointerTy() &&
	    llvm::isAllocaPromotable(var.addr)) {
	    index[var.addr] = tracked.size();
	    tracked.push_back(&var);
	}
    }
    if (tracked.empty()) {
	return;
    }

    llvm::DenseMap<const llvm::BasicBlock *, StateVec> in;
    std::vector<const llvm::BasicBlock *> pending{&fn.getEntryBlock()};
    in[pending[0]].assign(tracked.size(), UNKNOWN);

        while (!pending.empty()) {
	auto bb = pending.back();
	pending.pop_back();

	auto out = in[bb];
	auto r = transfer(*bb, out, index, [](const Report &) {});
	if (r.noReturn) {
	    continue;
	}
	auto term = bb->getTerminator();
	for (unsigned i = 0; i < term->getNumSuccessors(); ++i) {
	    auto succ = term->getSuccessor(i);
	    auto edge = out;
	    if (!refine(bb, i, r, edge)) {
		continue;
	    }
	    bool changed = !in.contains(succ);
	    auto &succIn = in[succ];
	    if (changed) {
		succIn.assign(tracked.size(), 0);
	    }
	    for (std::size_t v = 0; v < edge.size(); ++v) {
		if ((succIn[v] | edge[v]) != succIn[v]) {
		    succIn[v] |= edge[v];
		    changed = true;
		}
	    }
	    if (changed) {
		pending.push_back(succ);
	    }
	}
    }

    std::vector<Report> report;
    std::set<std::pair<std::size_t, int>> seen;
    llvm::ReversePostOrderTraversal<const llvm::Function *> rpo{&fn};
    for (auto bb : rpo) {
	if (!in.contains(bb)) {
	    continue;
	}
	auto state = in[bb];
	transfer(*bb, state, index,
	         [&](const Report &r) { report.push_back(r); });
    }
    auto locOf = [&](const Report &r) { return readLoc.find(r.read)->second; };
    report.erase(std::remove_if(report.begin(), report.end(),
                                [&](const Report &r) {
	                            return readLoc.find(r.read) == readLoc.end();
                                }),
                 report.end());
    std::stable_sort(report.begin(), report.end(),
                     [&](const Report &a, const Report &b) {
	                 auto la = locOf(a), lb = locOf(b);
	                 return std::tie(la.from.line, la.from.col) <
	                        std::tie(lb.from.line, lb.from.col);
                     });
    for (const auto &r : report) {
	if (!seen.insert({r.var, int(r.kind)}).second) {
	    continue;
	}
	auto loc = locOf(r);
	std::string what;
	auto name = tracked[r.var]->name;
	switch (r.kind) {
	case ReportKind::NULL_DEREF:
	    what = r.state == IS_NULL ? " is null when dereferenced"
	                              : " may be null when dereferenced";
	    break;
	case ReportKind::FREED_DEREF:
	    what = r.state == FREED ? " is dereferenced after free"
	                            : " may be dereferenced after free";
	    break;
	case ReportKind::DOUBLE_FREE:
	    what = r.state == FREED ? " is freed twice" : " may be freed twice";
	    break;
	}
	error::location(loc);
	error::out() << error::setColor(error::BOLD) << loc << ": "
	             << error::setColor(error::BOLD_BLUE)
	             << "warning: " << error::setColor(error::BOLD) << "pointer '"
	             << name << "'" << what << "\n"
	             << error::setColor(error::NORMAL);
	error::location(tracked[r.var]->loc);
	error::out() << error::setColor(error::BOLD) << tracked[r.var]->loc
	             << ": " << error::setColor(error::BOLD_BLUE)
	             << "note: " << error::setColor(error::BOLD)
	             << "variable declared here\n"
	             << error::setColor(error::NORMAL);
    }
}

} // namespace analysis
} // namespace abc
