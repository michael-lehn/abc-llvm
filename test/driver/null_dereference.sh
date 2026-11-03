# TEST: T0287
# -Wnull-dereference: every line of the program that must get a warning says so
# in a comment.  The function 'flag' is a known limit of the analysis: the result
# of the test is kept in a variable.

cat > cases.abc <<'EOF'
extern fn malloc(size: u64): -> void;
extern fn calloc(n: u64, size: u64): -> void;
extern fn free(p: -> void);
extern fn exit(c: int);
extern fn abort();
extern fn use(p: -> int);
extern fn cond(): int;
struct S { a: int; n: -> S; };
extern fn find(): -> S;

fn ptr1() { local p: -> int = malloc(4); *p = 1; } // WARN: may be null when dereferenced
fn ptr2() { local p: -> int = malloc(4); if (p) { *p = 1; } } // ok
fn ptr3() { local p: -> int = malloc(4); if (p) { *p = 1; free(p); } free(p); } // WARN: may be freed twice
fn isNull() { local p: -> int = nullptr; *p = 1; } // WARN: is null when dereferenced
fn notCheck(): int { local p: -> int = malloc(4); if (!p) { return 1; } *p = 1; return 0; } // ok
fn eqCheck(): int { local p: -> int = malloc(4); if (p == nullptr) { return 1; } *p = 1; return 0; } // ok
fn neCheck() { local p: -> int = malloc(4); if (p != nullptr) { *p = 1; } } // ok
fn wrongBranch() { local p: -> int = malloc(4); if (!p) { *p = 1; } } // WARN: is null when dereferenced
fn andCheck(): int { local s: -> S = malloc(16); if (s && s->a) { return 1; } return 0; } // ok
fn orCheck(): int { local s: -> S = malloc(16); if (!s || s->a) { return 1; } return s->a; } // ok
fn ternary(): int { local p: -> int = malloc(4); return p ? *p : 0; } // ok
fn whileCheck() { local p: -> int = malloc(4); while (p) { *p = 1; p = nullptr; } } // ok
fn assignInCond(): int { local p: -> int; if ((p = malloc(4))) { return *p; } return 0; } // ok
fn afterExit(): int { local p: -> int = malloc(4); if (!p) { exit(1); } return *p; } // ok
fn afterAbort(): int { local p: -> int = malloc(4); if (!p) { abort(); } return *p; } // ok
fn afterAssert(): int { local p: -> int = malloc(4); assert(p); return *p; } // ok
fn member(): int { local s: -> S = malloc(16); return s->a; } // WARN: may be null when dereferenced
fn index(): int { local p: -> int = malloc(4); return p[2]; } // WARN: may be null when dereferenced
fn arith(): int { local p: -> int = malloc(4); return *(p + 1); } // WARN: may be null when dereferenced
fn addrOfMember(): -> int { local s: -> S = malloc(16); return &s->a; } // ok (no access)
fn calloced() { local p: -> int = calloc(1, 4); *p = 1; } // WARN: may be null when dereferenced
fn once() { local p: -> int = malloc(4); *p = 1; *p = 2; } // WARN: may be null when dereferenced
fn reassigned() { local p: -> int = malloc(4); local x: int; p = &x; *p = 1; } // ok
fn merge() { local p: -> int = nullptr; local x: int; if (cond()) { p = &x; } *p = 1; } // WARN: may be null when dereferenced
fn loopAlloc() { local p: -> int = nullptr; for (local i: int = 0; i < 3; ++i) { p = malloc(4); if (!p) { return; } *p = i; } } // ok
fn passAlong() { local p: -> int = malloc(4); use(p); } // ok (no deref here)
fn escaped() { local p: -> int = malloc(4); local q: -> -> int = &p; *q = nullptr; *p = 1; } // ok (not tracked)
fn parameter(p: -> int): int { return *p; } // ok (unknown)
fn afterFree(): int { local p: -> int = malloc(4); if (!p) { return 0; } free(p); return *p; } // WARN: is dereferenced after free
fn doubleFree() { local p: -> int = malloc(4); free(p); free(p); } // WARN: is freed twice
fn twoChecks(c: int) { local p: -> int = malloc(4); if (p) { c = 1; } if (p) { *p = c; } } // ok
fn freeNull() { local p: -> int = malloc(4); free(p); p = nullptr; free(p); } // ok
fn alias() { local p: -> int = malloc(4); local q: -> int = p; *q = 1; } // missed (known limit)
fn flag() { local p: -> int = malloc(4); local ok: bool = p != nullptr; if (ok) { *p = 1; } } // WARN: may be null when dereferenced
fn unknownAfterLoop(list: -> S) { local n: -> S = list; for (; n; n = n->n) { n->a = 1; } n = malloc(16); n->a = 1; } // WARN: may be null when dereferenced
fn unknownNull(list: -> S): int { local n: -> S = list; if (!n) { return n->a; } return 0; } // WARN: is null when dereferenced
fn unknownTestedThenUsed(c: int): int { local n: -> S = find(); if (n) { c = 1; } return n->a + c; } // WARN: may be null when dereferenced
fn unknownUsed(): int { local n: -> S = find(); return n->a; } // ok
fn deadDeref() { local p: -> int = malloc(4); return; *p = 1; } // ok (dead)
fn main(): int { return 0; }
EOF

"$ABC" $ABC_FLAGS -Wnull-dereference -c cases.abc -o cases.o 2> err.txt || exit 1
sed -n "s/^.*cases\.abc:\([0-9]*\)\.[^:]*: .*warning: .*pointer '[a-z]*' \(.*\)$/\1 \2/p" \
    err.txt | sort > got.txt
sed -n '/WARN: /{=;s/^.*WARN: //;p;}' cases.abc | paste -d' ' - - | sort > expected.txt
cat err.txt
if ! diff expected.txt got.txt; then
    exit 1
fi

# no analysis without the option, and no change of the generated code
"$ABC" $ABC_FLAGS -c cases.abc -o default.o 2> err.txt || exit 1
if grep -q "warning: .*pointer" err.txt; then
    cat err.txt
    exit 1
fi
"$ABC" $ABC_FLAGS --emit-llvm cases.abc -o default.ll 2> /dev/null || exit 1
"$ABC" $ABC_FLAGS -Wnull-dereference --emit-llvm cases.abc -o enabled.ll 2> /dev/null ||
    exit 1
cmp default.ll enabled.ll
