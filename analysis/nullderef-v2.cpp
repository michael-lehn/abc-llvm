// -Wnull-dereference=strict. Same idea as nullderef.cpp, which is the simpler one
// to read first, with these extensions:
//
// (1) Tests kept in a variable: 'ok = p != nullptr; if (ok) *p = 1;'. A fact
//     "if ok is true then p is not null" is part of the state of the analysis.
// (2) Members: 'list->next->val'. The state belongs to an access path (list,
//     list->next, ...) instead of a variable. A path gets forgotten if it can
//     have changed: stores through other pointers and calls of unknown functions.
// (3) A member nothing is known about may be null. This one assumption is what
//     makes it strict: a member has to be tested before it is used, even if the
//     program guarantees in another way that it is valid.
// (4) A test after a dereference: 'if (list->next && list)'. Nothing is known
//     about 'list', so its dereference gets no warning, but the test shows that
//     it can be null. What is known afterwards is marked as coming from a
//     dereference, and the test gets the warning.

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#ifdef SUPPORT_SOLARIS
// has to be included as first llvm header
#include "llvm/Support/Solaris/sys/regset.h"
#endif // SUPPORT_SOLARIS

#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"

#include "lexer/error.hpp"

#include "nullderef-v2.hpp"

namespace abc {
namespace analysis {

namespace {

using State = std::uint8_t;
constexpr State IS_NULL = 1, NON_NULL = 2, FREED = 4, UNKNOWN = 8;
// not null only because an unknown pointer was dereferenced without a test
constexpr State DEREFERENCED = 16;

struct Step
{
	const llvm::Type *type;
	unsigned field;
	auto operator<=>(const Step &) const = default;
};

struct Path
{
	std::size_t root;
	std::vector<Step> steps;
	auto operator<=>(const Path &) const = default;
	bool hasPrefix(const Path &p) const
	{
	    return root == p.root && steps.size() >= p.steps.size() &&
	           std::equal(p.steps.begin(), p.steps.end(), steps.begin());
	}
};

struct Fact
{
	Path path;
	bool nonNullIfTrue;
	bool operator==(const Fact &) const = default;
};

struct Env
{
	std::map<Path, State> state;
	std::map<std::size_t, Fact> fact; // integer variable -> fact
	bool operator==(const Env &) const = default;
};

// what is known about an SSA value within a block
struct ValueInfo
{
	const Path *holds = nullptr;    // value of this path
	const Path *addr = nullptr;     // address of this path
	const Path *derefBase = nullptr; // derived from the value of this path
	const llvm::Value *read = nullptr; // load of derefBase
	const Fact *cmp = nullptr;      // comparison with null
	std::size_t intRoot = ~std::size_t(0);
};

struct Root
{
	const llvm::AllocaInst *addr;
	std::string name;
	bool viaPointer;
};

struct Report
{
	const llvm::Value *read;
	std::string text;
	State state;
	int kind;
};

} // namespace

static const std::vector<Root> *roots;

static State
get(const Env &env, const Path &p)
{
    auto it = env.state.find(p);
    if (it != env.state.end()) {
	return it->second;
    }
    if (!p.steps.empty() && (*roots)[p.root].viaPointer) {
	return IS_NULL | NON_NULL;
    }
    return UNKNOWN;
}

static void
forget(Env &env, const Path &p, bool withPrefix)
{
    for (auto it = env.state.begin(); it != env.state.end();) {
	if (withPrefix ? it->first.hasPrefix(p) : it->first == p) {
	    it = env.state.erase(it);
	} else {
	    ++it;
	}
    }
    for (auto it = env.fact.begin(); it != env.fact.end();) {
	if (it->second.path.hasPrefix(p)) {
	    it = env.fact.erase(it);
	} else {
	    ++it;
	}
    }
}

static void
forgetMemory(Env &env)
{
    for (auto it = env.state.begin(); it != env.state.end();) {
	if (!it->first.steps.empty() && (*roots)[it->first.root].viaPointer) {
	    it = env.state.erase(it);
	} else {
	    ++it;
	}
    }
    for (auto it = env.fact.begin(); it != env.fact.end();) {
	const auto &p = it->second.path;
	if (!p.steps.empty() && (*roots)[p.root].viaPointer) {
	    it = env.fact.erase(it);
	} else {
	    ++it;
	}
    }
}

static bool
named(const llvm::CallInst *call, std::initializer_list<const char *> names)
{
    auto fn = call->getCalledFunction();
    if (!fn) {
	return false;
    }
    return std::any_of(names.begin(), names.end(),
                       [&](const char *n) { return fn->getName() == n; });
}

static State
stateOfValue(const llvm::Value *val)
{
    if (llvm::isa<llvm::ConstantPointerNull>(val)) {
	return IS_NULL;
    }
    if (auto call = llvm::dyn_cast<llvm::CallInst>(val)) {
	if (named(call, {"malloc", "calloc", "realloc"})) {
	    return IS_NULL | NON_NULL;
	}
    }
    if (llvm::isa<llvm::AllocaInst>(val) || llvm::isa<llvm::GlobalValue>(val)) {
	return NON_NULL;
    }
    return UNKNOWN;
}

// A variable can be a root if its address is only used for loads and stores of
// it or of its members: nothing else can then change it.
static bool
onlyDirectAccess(const llvm::Value *addr)
{
    for (const auto *user : addr->users()) {
	if (auto load = llvm::dyn_cast<llvm::LoadInst>(user)) {
	    if (load->getPointerOperand() != addr) {
		return false;
	    }
	} else if (auto store = llvm::dyn_cast<llvm::StoreInst>(user)) {
	    if (store->getValueOperand() == addr) {
		return false;
	    }
	} else if (auto gep = llvm::dyn_cast<llvm::GetElementPtrInst>(user)) {
	    if (!gep->getSourceElementType()->isStructTy() ||
	        !gep->hasAllConstantIndices() || gep->getNumIndices() != 2 ||
	        !onlyDirectAccess(gep)) {
		return false;
	    }
	} else {
	    return false;
	}
    }
    return true;
}

struct Block
{
	llvm::DenseMap<const llvm::Value *, ValueInfo> info;
	std::vector<std::unique_ptr<Path>> paths;
	std::vector<std::unique_ptr<Fact>> facts;
	bool noReturn = false;

	const Path *keep(Path p)
	{
	    paths.push_back(std::make_unique<Path>(std::move(p)));
	    return paths.back().get();
	}
};

template <typename OnReport>
static void
transfer(const llvm::BasicBlock &bb, Env &env, Block &b,
         const llvm::DenseMap<const llvm::Value *, std::size_t> &rootIndex,
         const llvm::DenseMap<const llvm::Value *, std::string> &text,
         OnReport onReport)
{
    auto infoOf = [&](const llvm::Value *v) -> ValueInfo {
	auto it = b.info.find(v);
	if (it != b.info.end()) {
	    return it->second;
	}
	auto r = rootIndex.find(v);
	if (r != rootIndex.end()) {
	    ValueInfo i;
	    i.addr = b.keep(Path{r->second, {}});
	    return i;
	}
	return ValueInfo{};
    };
    auto describe = [&](const ValueInfo &i) {
	auto t = text.find(i.read);
	return t != text.end() ? t->second : (*roots)[i.derefBase->root].name;
    };
    auto deref = [&](const llvm::Value *ptr) {
	auto i = infoOf(ptr);
	if (!i.derefBase) {
	    return;
	}
	auto s = get(env, *i.derefBase);
	if (s & FREED) {
	    onReport(Report{i.read, describe(i), s, 1});
	} else if (s & IS_NULL) {
	    onReport(Report{i.read, describe(i), s, 0});
	}
	forget(env, *i.derefBase, false);
	env.state[*i.derefBase] =
	    NON_NULL | ((s & (UNKNOWN | DEREFERENCED)) ? DEREFERENCED : 0);
    };

    for (const auto &inst : bb) {
	if (auto store = llvm::dyn_cast<llvm::StoreInst>(&inst)) {
	    auto ptr = store->getPointerOperand();
	    auto val = store->getValueOperand();
	    deref(ptr);
	    auto i = infoOf(ptr);
	    if (i.addr) {
		const auto &p = *i.addr;
		// a copy of a member: its value is what is known about it. To be
		// read first, as 'l = l->next' forgets the paths of l.
		auto copied = infoOf(val);
		State newState =
		    copied.holds && !copied.holds->steps.empty() &&
		            (*roots)[copied.holds->root].viaPointer
		        ? get(env, *copied.holds)
		        : stateOfValue(val);
		forget(env, p, true);
		// another pointer can refer to the same member
		if (!p.steps.empty() && (*roots)[p.root].viaPointer) {
		    auto last = p.steps.back();
		    for (auto it = env.state.begin(); it != env.state.end();) {
			if (!it->first.steps.empty() &&
			    it->first.steps.back() == last) {
			    it = env.state.erase(it);
			} else {
			    ++it;
			}
		    }
		}
		if (val->getType()->isPointerTy()) {
		    auto v = infoOf(val);
		    env.state[p] = newState;
		    v.holds = b.keep(p);
		    b.info[val] = v;
		} else if (val->getType()->isIntegerTy() && p.steps.empty()) {
		    const llvm::Value *source = val;
		    if (auto z = llvm::dyn_cast<llvm::ZExtInst>(source)) {
			source = z->getOperand(0);
		    }
		    env.fact.erase(p.root);
		    auto s = infoOf(source);
		    if (s.cmp) {
			env.fact[p.root] = *s.cmp;
		    }
		}
	    } else if (val->getType()->isPointerTy() ||
	               val->getType()->isAggregateType()) {
		forgetMemory(env);
	    }
	} else if (auto load = llvm::dyn_cast<llvm::LoadInst>(&inst)) {
	    auto ptr = load->getPointerOperand();
	    deref(ptr);
	    auto i = infoOf(ptr);
	    if (i.addr) {
		ValueInfo v;
		if (load->getType()->isPointerTy()) {
		    v.holds = i.addr;
		    v.derefBase = i.addr;
		    v.read = load;
		} else if (load->getType()->isIntegerTy() &&
		           i.addr->steps.empty()) {
		    v.intRoot = i.addr->root;
		}
		b.info[load] = v;
	    }
	} else if (auto gep = llvm::dyn_cast<llvm::GetElementPtrInst>(&inst)) {
	    auto i = infoOf(gep->getPointerOperand());
	    ValueInfo v;
	    v.derefBase = i.derefBase;
	    v.read = i.read;
	    const Path *from = i.addr ? i.addr : i.holds;
	    if (from && gep->getSourceElementType()->isStructTy() &&
	        gep->hasAllConstantIndices() && gep->getNumIndices() == 2) {
		Path p = *from;
		auto last = llvm::cast<llvm::ConstantInt>(gep->getOperand(2));
		p.steps.push_back(
		    {gep->getSourceElementType(), unsigned(last->getZExtValue())});
		v.addr = b.keep(std::move(p));
	    }
	    b.info[gep] = v;
	} else if (auto icmp = llvm::dyn_cast<llvm::ICmpInst>(&inst)) {
	    auto lhs = icmp->getOperand(0), rhs = icmp->getOperand(1);
	    if (llvm::isa<llvm::ConstantPointerNull>(lhs)) {
		std::swap(lhs, rhs);
	    }
	    auto i = infoOf(lhs);
	    if (i.holds && llvm::isa<llvm::ConstantPointerNull>(rhs) &&
	        icmp->isEquality()) {
		auto s = get(env, *i.holds);
		if ((s & DEREFERENCED) && i.read) {
		    onReport(Report{i.read, describe(i), s, 3});
		}
		b.facts.push_back(std::make_unique<Fact>(
		    Fact{*i.holds,
		         icmp->getPredicate() == llvm::ICmpInst::ICMP_NE}));
		ValueInfo v;
		v.cmp = b.facts.back().get();
		b.info[icmp] = v;
	    }
	} else if (auto call = llvm::dyn_cast<llvm::CallInst>(&inst)) {
	    if (named(call, {"free"}) && call->arg_size() == 1) {
		auto i = infoOf(call->getArgOperand(0));
		if (i.holds) {
		    auto s = get(env, *i.holds);
		    if ((s & FREED) && i.read) {
			onReport(Report{i.read, describe(i), s, 2});
		    }
		    forget(env, *i.holds, true);
		    env.state[*i.holds] = FREED;
		}
	    }
	    for (unsigned a = 0; a < call->arg_size(); ++a) {
		if (call->paramHasAttr(a, llvm::Attribute::ByVal)) {
		    deref(call->getArgOperand(a));
		}
	    }
	    if (named(call, {"exit", "abort", "_Exit", "__assert"})) {
		b.noReturn = true;
		return;
	    }
	    if (!named(call, {"malloc", "calloc", "realloc", "free", "printf",
	                      "strlen", "strcmp"})) {
		forgetMemory(env);
	    }
	}
    }
}

static bool
refine(const llvm::BasicBlock *bb, unsigned succNo, Block &b, Env &env)
{
    auto br = bb->getTerminator();
    if (br->getNumSuccessors() != 2 || br->getNumOperands() != 3) {
	return true;
    }
    const llvm::Value *cond = br->getOperand(0);
    if (auto c = llvm::dyn_cast<llvm::ConstantInt>(cond)) {
	return succNo == (c->isZero() ? 1u : 0u);
    }
    bool ifTrue = succNo == 0;
    for (;;) {
	auto it = b.info.find(cond);
	const Fact *fact = nullptr;
	if (it != b.info.end() && it->second.cmp) {
	    fact = it->second.cmp;
	} else if (it != b.info.end() &&
	           it->second.intRoot != ~std::size_t(0)) {
	    auto f = env.fact.find(it->second.intRoot);
	    if (f != env.fact.end()) {
		fact = &f->second;
	    }
	}
	if (fact) {
	    bool nonNull = fact->nonNullIfTrue == ifTrue;
	    auto path = fact->path;
	    State s = get(env, path);
	    if (nonNull) {
		s = (s & (NON_NULL | FREED)) | ((s & UNKNOWN) ? NON_NULL : 0);
	    } else {
		s = (s & (IS_NULL | UNKNOWN)) ? IS_NULL : 0;
	    }
	    env.state[path] = s;
	    return s != 0;
	}
	auto icmp = llvm::dyn_cast<llvm::ICmpInst>(cond);
	if (!icmp || !icmp->isEquality()) {
	    return true;
	}
	auto ci = llvm::dyn_cast<llvm::ConstantInt>(icmp->getOperand(1));
	if (!ci || !ci->isZero()) {
	    return true;
	}
	bool isNe = icmp->getPredicate() == llvm::ICmpInst::ICMP_NE;
	ifTrue = ifTrue == isNe;
	cond = icmp->getOperand(0);
    }
}

static bool
join(Env &into, const Env &from, bool first)
{
    if (first) {
	into = from;
	return true;
    }
    Env old = into;
    std::set<Path> keys;
    for (const auto &[p, s] : into.state) {
	keys.insert(p);
    }
    for (const auto &[p, s] : from.state) {
	keys.insert(p);
    }
    std::map<Path, State> merged;
    for (const auto &p : keys) {
	merged[p] = get(old, p) | get(from, p);
    }
    into.state = std::move(merged);
    for (auto it = into.fact.begin(); it != into.fact.end();) {
	auto f = from.fact.find(it->first);
	if (f == from.fact.end() || !(f->second == it->second)) {
	    it = into.fact.erase(it);
	} else {
	    ++it;
	}
    }
    return !(into == old);
}

void
checkNullDereferenceStrict(llvm::Function &fn, const SourceInfo &info)
{
    const auto &text = info.readText;
    std::vector<Root> rootList;
    llvm::DenseMap<const llvm::Value *, std::size_t> rootIndex;
    for (const auto &inst : fn.getEntryBlock()) {
	auto alloca = llvm::dyn_cast<llvm::AllocaInst>(&inst);
	if (!alloca || !onlyDirectAccess(alloca)) {
	    continue;
	}
	std::string name = alloca->getName().str();
	if (name == ".retVal" || name.empty()) {
	    continue;
	}
	// '.function.name.number'
	auto end = name.rfind('.');
	auto begin = name.rfind('.', end - 1);
	if (end != std::string::npos && begin != std::string::npos) {
	    name = name.substr(begin + 1, end - begin - 1);
	}
	rootIndex[alloca] = rootList.size();
	rootList.push_back(
	    {alloca, name, alloca->getAllocatedType()->isPointerTy()});
    }
    roots = &rootList;

    llvm::DenseMap<const llvm::BasicBlock *, Env> in;
    std::vector<const llvm::BasicBlock *> pending{&fn.getEntryBlock()};
    in[pending[0]] = Env{};
    while (!pending.empty()) {
	auto bb = pending.back();
	pending.pop_back();
	Env out = in[bb];
	Block b;
	transfer(*bb, out, b, rootIndex, text, [](const Report &) {});
	if (b.noReturn) {
	    continue;
	}
	auto term = bb->getTerminator();
	for (unsigned i = 0; i < term->getNumSuccessors(); ++i) {
	    Env edge = out;
	    if (!refine(bb, i, b, edge)) {
		continue;
	    }
	    auto succ = term->getSuccessor(i);
	    bool first = !in.contains(succ);
	    if (join(in[succ], edge, first)) {
		pending.push_back(succ);
	    }
	}
    }

    std::vector<Report> report;
    llvm::ReversePostOrderTraversal<const llvm::Function *> rpo{&fn};
    for (auto bb : rpo) {
	if (!in.contains(bb)) {
	    continue;
	}
	Env env = in[bb];
	Block b;
	transfer(*bb, env, b, rootIndex, text,
	         [&](const Report &r) { report.push_back(r); });
    }
    const auto &readLoc = info.readLoc;
    report.erase(std::remove_if(report.begin(), report.end(),
                                [&](const Report &r) {
	                            return readLoc.find(r.read) == readLoc.end();
                                }),
                 report.end());
    auto locOf = [&](const Report &r) { return readLoc.find(r.read)->second; };
    std::stable_sort(report.begin(), report.end(),
                     [&](const Report &x, const Report &y) {
	                 auto a = locOf(x), c = locOf(y);
	                 return std::tie(a.from.line, a.from.col) <
	                        std::tie(c.from.line, c.from.col);
                     });
    std::set<std::pair<std::string, int>> seen;
    for (const auto &r : report) {
	if (!seen.insert({r.text, r.kind}).second) {
	    continue;
	}
	auto loc = locOf(r);
	static const char *const maybe[] = {" may be null when dereferenced",
	                                    " may be dereferenced after free",
	                                    " may be freed twice",
	                                    " is tested for null after it may have"
	                                    " been dereferenced"};
	static const char *const sure[] = {" is null when dereferenced",
	                                   " is dereferenced after free",
	                                   " is freed twice",
	                                   " is tested for null after it was"
	                                   " dereferenced"};
	bool isSure = r.kind == 3 ? !(r.state & ~(NON_NULL | DEREFERENCED))
	                          : r.state == (r.kind == 0 ? IS_NULL : FREED);
	error::location(loc);
	error::out() << error::setColor(error::BOLD) << loc << ": "
	             << error::setColor(error::BOLD_BLUE)
	             << "warning: " << error::setColor(error::BOLD) << "pointer '"
	             << r.text << "'" << (isSure ? sure : maybe)[r.kind] << "\n"
	             << error::setColor(error::NORMAL);
    }
}

} // namespace analysis
} // namespace abc
