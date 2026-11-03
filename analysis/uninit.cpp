#include <algorithm>
#include <cstdint>
#include <tuple>
#include <vector>

#ifdef SUPPORT_SOLARIS
// has to be included as first llvm header
#include "llvm/Support/Solaris/sys/regset.h"
#endif // SUPPORT_SOLARIS

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"

#include "lexer/error.hpp"

#include "uninit.hpp"

namespace abc {
namespace analysis {

namespace opt {

bool uninitialized = false;

} // namespace opt

namespace {

struct Local
{
    llvm::AllocaInst *addr;
    UStr name;
    lexer::Loc loc;
};

// Set of states a variable can be in on the paths reaching a program point
using State = std::uint8_t;
constexpr State UNASSIGNED = 1, ASSIGNED = 2;
using StateVec = std::vector<State>;

} // namespace

static std::vector<Local> local;

// A ValueMap as instructions in unreachable blocks get deleted
static llvm::ValueMap<const llvm::Value *, lexer::Loc> readLoc;

void
declareLocal(gen::Value addr, UStr name, lexer::Loc loc)
{
    if (!opt::uninitialized) {
	return;
    }
    auto alloca = llvm::cast<llvm::AllocaInst>(addr);
    local.push_back({alloca, name, loc});
}

void
noteRead(gen::Value load, lexer::Loc loc)
{
    if (!opt::uninitialized) {
	return;
    }
    readLoc[load] = loc;
}

// Successors that can actually be reached. Code generation keeps branches on
// constants, e.g. for 'while (1)' or 'if (0)'
static std::vector<const llvm::BasicBlock *>
feasibleSuccessors(const llvm::BasicBlock *bb)
{
    auto term = bb->getTerminator();
    if (term->getNumSuccessors() == 2 && term->getNumOperands() == 3) {
	if (auto c = llvm::dyn_cast<llvm::ConstantInt>(term->getOperand(0))) {
	    return {term->getSuccessor(c->isZero() ? 1 : 0)};
	}
    }
    return {llvm::succ_begin(bb), llvm::succ_end(bb)};
}

template <typename OnRead>
static void
transfer(const llvm::BasicBlock &bb, StateVec &state,
         const llvm::DenseMap<const llvm::Value *, std::size_t> &index,
         OnRead onRead)
{
    for (const auto &inst : bb) {
	if (auto store = llvm::dyn_cast<llvm::StoreInst>(&inst)) {
	    auto it = index.find(store->getPointerOperand());
	    if (it != index.end()) {
		state[it->second] = ASSIGNED;
	    }
	} else if (auto load = llvm::dyn_cast<llvm::LoadInst>(&inst)) {
	    auto it = index.find(load->getPointerOperand());
	    if (it != index.end()) {
		onRead(load, it->second, state[it->second]);
	    }
	}
    }
}

void
checkUninitialized(llvm::Function &fn)
{
    if (!opt::uninitialized) {
	return;
    }

    // Only variables whose address never escapes: every access is a plain load
    // or store of the whole variable, so the CFG shows all reads and writes
    std::vector<const Local *> tracked;
    llvm::DenseMap<const llvm::Value *, std::size_t> index;
    for (const auto &var : local) {
	if (llvm::isAllocaPromotable(var.addr)) {
	    index[var.addr] = tracked.size();
	    tracked.push_back(&var);
	}
    }

    if (!tracked.empty()) {
	llvm::DenseMap<const llvm::BasicBlock *, StateVec> in;
	std::vector<const llvm::BasicBlock *> pending{&fn.getEntryBlock()};
	in[pending[0]].assign(tracked.size(), UNASSIGNED);

	auto ignoreRead = [](auto, auto, auto) {};
	while (!pending.empty()) {
	    auto bb = pending.back();
	    pending.pop_back();
	    auto out = in[bb];
	    transfer(*bb, out, index, ignoreRead);
	    for (auto succ : feasibleSuccessors(bb)) {
		bool changed = !in.contains(succ);
		auto &succIn = in[succ];
		if (changed) {
		    succIn.assign(tracked.size(), 0);
		}
		for (std::size_t i = 0; i < out.size(); ++i) {
		    if ((succIn[i] | out[i]) != succIn[i]) {
			succIn[i] |= out[i];
			changed = true;
		    }
		}
		if (changed) {
		    pending.push_back(succ);
		}
	    }
	}

	// Report the first read of each variable in source order
	struct Report
	{
	    lexer::Loc loc;
	    State state;
	    bool valid = false;
	};
	std::vector<Report> report(tracked.size());
	auto before = [](const lexer::Loc &a, const lexer::Loc &b) {
	    return std::tie(a.from.line, a.from.col) <
	           std::tie(b.from.line, b.from.col);
	};
	llvm::ReversePostOrderTraversal<const llvm::Function *> rpo{&fn};
	for (auto bb : rpo) {
	    if (!in.contains(bb)) {
		continue;
	    }
	    auto state = in[bb];
	    transfer(*bb, state, index,
	             [&](const llvm::LoadInst *load, std::size_t i, State s) {
		         auto it = readLoc.find(load);
		         if (!(s & UNASSIGNED) || it == readLoc.end()) {
			     return;
		         }
		         auto &r = report[i];
		         if (!r.valid || before(it->second, r.loc)) {
			     r = {it->second, s, true};
		         }
	             });
	}

	std::vector<std::size_t> order;
	for (std::size_t i = 0; i < report.size(); ++i) {
	    if (report[i].valid) {
		order.push_back(i);
	    }
	}
	std::sort(order.begin(), order.end(), [&](auto a, auto b) {
	    return before(report[a].loc, report[b].loc);
	});
	for (auto i : order) {
	    const auto &r = report[i];
	    error::location(r.loc);
	    error::out() << error::setColor(error::BOLD) << r.loc << ": "
	                 << error::setColor(error::BOLD_BLUE)
	                 << "warning: " << error::setColor(error::BOLD)
	                 << "variable '" << tracked[i]->name << "' "
	                 << (r.state == UNASSIGNED ? "is" : "may be")
	                 << " used uninitialized\n"
	                 << error::setColor(error::NORMAL);
	    error::location(tracked[i]->loc);
	    error::out() << error::setColor(error::BOLD) << tracked[i]->loc
	                 << ": " << error::setColor(error::BOLD_BLUE)
	                 << "note: " << error::setColor(error::BOLD)
	                 << "variable declared here\n"
	                 << error::setColor(error::NORMAL);
	}
    }

    local.clear();
    readLoc.clear();
}

} // namespace analysis
} // namespace abc
