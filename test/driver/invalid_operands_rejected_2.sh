# TEST: T0322
# More invalid programs that get an ordinary error, not a crash and not an internal
# compiler error: pointers to something without a size, compound assignment with
# a pointer on the right, literals without a size, members of what has none.

status=0
reject()
{
    echo "$1" > bad.abc
    "$ABC" $ABC_FLAGS -c bad.abc -o bad.o > err.txt 2>&1
    code=$?
    if [ $code -ne 1 ] || ! grep -q 'error' err.txt ||
	grep -q 'internal compiler error' err.txt; then
	echo "exit status $code for: $1"
	cat err.txt
	status=1
    fi
}

accept()
{
    echo "$1" > good.abc
    if ! "$ABC" $ABC_FLAGS -c good.abc -o good.o > err.txt 2>&1; then
	echo "rejected: $1"
	cat err.txt
	status=1
    fi
}

F='struct F;'
fn='fn f() {}'
S='struct S { a: int; };'

# pointer to a struct without definition
reject "$F fn main() { local p: -> F; *p; }"
reject "$F fn main() { local p: -> F; p[0]; }"
reject "$F fn main() { local p: -> F; p = p + 1; }"
reject "$F fn main() { local p: -> F; p = 1 + p; }"
reject "$F fn main() { local p: -> F; ++p; }"
reject "$F fn main() { local p: -> F; p--; }"
reject "$F fn main() { local p: -> F; p += 1; }"
reject "$F fn main() { local p: -> F; local d: i64 = p - p; }"
reject "$F fn main() { local p: -> F; p->a; }"
reject "$F fn main() { sizeof(F); }"
reject "$F fn main() { local p: -> F; sizeof(*p); }"
reject "$F global x: u64 = sizeof(F);"
accept "$F fn main() { local p: -> F; local q: -> F = p; local b: bool = p == q; sizeof(p); }"
accept "$F fn g(p: -> F) {} struct F { a: int; }; fn main() { local x: F; g(&x); ++(&x)[0].a; }"

# function pointer
reject "$fn fn main() { local p: -> fn() = &f; ++p; }"
reject "$fn fn main() { local p: -> fn() = &f; p--; }"
reject "$fn fn main() { local p: -> fn() = &f; p = p + 1; }"
reject "$fn fn main() { local p: -> fn() = &f; p += 1; }"
reject "$fn fn main() { local p: -> fn() = &f; p[0]; }"
reject "$fn fn main() { local p: -> fn() = &f; local d: i64 = p - p; }"
reject "$fn fn main() { &f(); }"
accept "$fn fn main() { local p: -> fn() = &f; *p; (*p)(); *&f; }"

# compound assignment with a pointer on the right
reject 'fn main() { local i: int; local p: -> int; i -= p; }'
reject 'fn main() { local i: int; local p: -> int; i += p; }'
reject 'fn main() { local d: double; local p: -> int; d -= p; }'
reject 'fn main() { local d: double; local a: array[2] of int; d -= a; }'
reject 'fn main() { local i: int; local a: array[2] of int; i -= a; }'
reject 'fn main() { local p, q: -> int; p -= q; }'
reject 'fn main() { local p, q: -> int; p += q; }'
accept 'fn main() { local p: -> int; p += 1; p -= 1; local i: i64 = p - p; }'

# initializers
reject 'fn main() { local z: array[] of int = {[4294967296] = 1}; }'
reject 'fn main() { local z: array[4294967296] of int = {[1] = 1}; }'
reject 'global z: array[4294967296] of int = {1};'
accept 'fn main() { local z: array[] of int = {[9999] = 1}; }'

# literals and members
reject "$F fn main() { F{}; }"
reject "$F fn main() { F{1}; }"
reject 'fn main() { void{}; }'
reject 'type V: void; fn main() { V{1}; }'
reject 'fn main() { nullptr->a; }'
reject 'fn main() { nullptr.a; }'
reject 'fn main() { (1).a; }'
reject 'fn main() { (1.5).a; }'
reject 'fn main() { local x: int; x.a; }'
reject "$S fn main() { local s: S; if (1 ? s : s) { } }"
reject "$S fn main() { local c: int = 1; local s: S; if (c ? s : s) { } }"
reject "$S fn main() { local c: int = 1; local s: S; while (c ? s : s) { } }"
reject "$S fn main() { local c: int = 1; local s: S; local b: bool = (c ? s : s) && 1; }"
reject 'fn main() { local c: int = 1; local a: array[2] of int; if (c ? a : a) { } }'

# void on either side, also with a floating point value
reject "$fn fn main() { local d: double; d = f(); }"
reject "$fn fn main() { local d: double = f() + 1.5; }"
reject "$fn fn main() { local d: double = 1.5 + f(); }"
reject "$fn fn main() { local d: double; d += f(); }"
reject "$fn fn main() { local a: array[2] of int; a[f()]; }"
reject "$fn fn main() { f()[1.5]; }"
reject "$fn fn main() { local b: bool = f() < 1.5; }"
reject "$fn fn main() { f() = 1.5; }"

# comparing addresses of functions is not a constant, but works at run time
reject "$fn global b: bool = &f == &f;"
accept "$fn fn main() { local b: bool = &f == &f; b = &f != &f; }"

# nullptr has a size
accept 'fn main() { local n: u64 = sizeof(nullptr); }'
accept 'global n: u64 = sizeof(nullptr);'
exit $status
