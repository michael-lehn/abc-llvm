# TEST: T0288
# -Wnull-dereference=strict: every line of the program that must get a warning
# says so in a comment.  Tests kept in a variable and members are known, and a
# member nothing is known about may be null.  The function 'flagSet' is a known
# limit: it would need to follow each path separately.  -Wall enables the strict
# check, and plain -Wnull-dereference knows nothing about members.

cat > cases.abc <<'EOF'
extern fn malloc(size: u64): -> void;
extern fn other();
struct N { val: int; next: -> N; };
struct H { first: -> N; };

fn okFlag() { local p: -> int = malloc(4); local ok: bool = p != nullptr; if (ok) { *p = 1; } } // ok
fn okInt() { local p: -> int = malloc(4); local ok: int = p != nullptr; if (ok) { *p = 1; } } // ok
fn okNot(): int { local p: -> int = malloc(4); local ok: bool = p != nullptr; if (!ok) { return 0; } return *p; } // ok
fn okEq(): int { local p: -> int = malloc(4); local bad: bool = p == nullptr; if (bad) { return 0; } return *p; } // ok
fn okWrong() { local p: -> int = malloc(4); local ok: bool = p != nullptr; if (!ok) { *p = 1; } } // WARN: is null when dereferenced
fn okStale() { local p: -> int = malloc(4); local ok: bool = p != nullptr; p = malloc(4); if (ok) { *p = 1; } } // WARN: may be null when dereferenced
fn okChanged(c: bool) { local p: -> int = malloc(4); local ok: bool = p != nullptr; ok = c; if (ok) { *p = 1; } } // WARN: may be null when dereferenced
fn okInBranch(c: int) { local p: -> int = malloc(4); local ok: bool = c != 0; if (c) { ok = p != nullptr; } if (ok) { *p = 1; } } // WARN: may be null when dereferenced
fn flagSet() { local p: -> int = malloc(4); local ok: bool = false; if (p) { ok = true; } if (ok) { *p = 1; } } // WARN: may be null when dereferenced
fn memberNull(): int { local n: -> N = malloc(16); if (!n) { return 0; } n->next = nullptr; return n->next->val; } // WARN: is null when dereferenced
fn memberMalloc(): int { local n: -> N = malloc(16); if (!n) { return 0; } n->next = malloc(16); return n->next->val; } // WARN: may be null when dereferenced
fn memberTested(): int { local n: -> N = malloc(16); if (!n) { return 0; } n->next = malloc(16); if (n->next) { return n->next->val; } return 0; } // ok
fn memberUnknown(list: -> N): int { return list->next->val; } // WARN: may be null when dereferenced
fn memberChecked(list: -> N): int { if (list->next) { return list->next->val; } return 0; } // ok
fn memberAnd(list: -> N): int { if (list->next && list->next->next) { return list->next->next->val; } return 0; } // ok
fn memberCall(list: -> N): int { if (list->next) { other(); return list->next->val; } return 0; } // WARN: may be null when dereferenced
fn memberCallMalloc(list: -> N): int { list->next = nullptr; other(); return list->next->val; } // WARN: may be null when dereferenced
fn memberAlias(a: -> N, b: -> N): int { a->next = nullptr; b->next = a; return a->next->val; } // WARN: may be null when dereferenced
fn memberReassign(list: -> N): int { list->next = nullptr; list = list; return list->val; } // ok
fn memberOverwritten(n: -> N): int { n->next = nullptr; local x: N; n->next = &x; return n->next->val; } // ok
fn walk(list: -> N): int { local s: int = 0; for (local l: -> N = list; l; l = l->next) { s += l->val; } return s; } // ok
fn walkNext(list: -> N): int { local l: -> N = list; while (l->next) { l = l->next; } return l->val; } // ok
fn nextOfNext(l: -> N): int { local n: -> N = l->next; return n->val; } // WARN: may be null when dereferenced
fn head(h: -> H): int { return h->first->val; } // WARN: may be null when dereferenced
fn localStruct(): int { local h: H; h.first = nullptr; return h.first->val; } // WARN: is null when dereferenced
fn localStructOk(): int { local h: H; h.first = nullptr; if (h.first) { return h.first->val; } return 0; } // ok
fn deep(): int { local n: -> N = malloc(16); if (!n) { return 0; } n->next = nullptr; n = malloc(16); if (!n) { return 0; } n->next = malloc(16); if (!n->next) { return 0; } return n->next->val; } // ok
fn main(): int { return 0; }
EOF

warnings()
{
    sed -n "s/^.*cases\.abc:\([0-9]*\)\.[^:]*: .*warning: .*pointer '.*' \(.*\)$/\1 \2/p" \
	"$1" | sort
}

"$ABC" $ABC_FLAGS -Wnull-dereference=strict -c cases.abc -o cases.o 2> err.txt || exit 1
warnings err.txt > got.txt
sed -n '/WARN: /{=;s/^.*WARN: //;p;}' cases.abc | paste -d' ' - - | sort > expected.txt
cat err.txt
diff expected.txt got.txt || exit 1

"$ABC" $ABC_FLAGS -Wall -c cases.abc -o cases.o 2> err.txt || exit 1
warnings err.txt > all.txt
diff expected.txt all.txt || exit 1

"$ABC" $ABC_FLAGS -Wnull-dereference -c cases.abc -o cases.o 2> err.txt || exit 1
if grep -q "pointer '.*->" err.txt; then
    echo "-Wnull-dereference reports members:"
    cat err.txt
    exit 1
fi

"$ABC" $ABC_FLAGS --emit-llvm cases.abc -o default.ll 2> /dev/null || exit 1
"$ABC" $ABC_FLAGS -Wnull-dereference=strict --emit-llvm cases.abc -o enabled.ll 2> /dev/null ||
    exit 1
cmp default.ll enabled.ll
