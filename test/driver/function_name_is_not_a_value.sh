# TEST: T0319
# The address of a function is &f. A function name alone is no value: this is an
# ordinary error, not a crash, and everything with &f stays valid.

status=0
pre='extern fn printf(s: -> readonly char, ...): int; fn f(): int { return 1; }
fn take(p: -> fn(): int): int { return (*p)(); }'

compile()
{
    printf '%s\n%s\n' "$pre" "$1" > t.abc
    "$ABC" $ABC_FLAGS -c t.abc -o t.o > err.txt 2>&1
}

reject()
{
    compile "$1"
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
    if ! compile "$1"; then
	echo "rejected: $1"
	cat err.txt
	status=1
    fi
}

reject 'fn main() { if (f) { } }'
reject 'fn main() { while (f) { } }'
reject 'fn main() { assert(f); }'
reject 'fn main() { local i: int = f ? 1 : 2; }'
reject 'fn main() { local b: bool = f && 1; }'
reject 'fn main() { local b: bool = !f; }'
reject 'fn main() { local d: double; d = f; }'
reject 'fn main() { local i: int; i = f; }'
reject 'fn main() { local i: int = f + 1; }'
reject 'fn main() { local d: double = 1.5 + f; }'
reject 'fn main() { local b: bool = f == f; }'
reject 'fn main() { local q: -> fn(): int = f; }'
reject 'fn main() { local q: -> fn(): int; q = f; }'
reject 'fn main() { local q: -> fn(): int = &f; local b: bool = q == f; }'
reject 'fn main() { take(f); }'
reject 'fn main() { printf("%p", f); }'
reject 'fn main() { local c: int = 1; c ? f : f; }'
reject 'global q: -> fn(): int = f;'
reject 'fn main() { local q: -> fn(): int = &f; if (*q) { } }'
reject 'fn main() { local q: -> fn(): int = &f; printf("%p", *q); }'

accept 'fn main() { if (&f) { } }'
accept 'fn main() { f(); }'
accept 'fn main() { local q: -> fn(): int = &f; if (q) { } (*q)(); }'
accept 'fn main() { local q: -> fn(): int = &f; q = &f; local b: bool = q == &f; }'
accept 'fn main() { take(&f); }'
accept 'fn main() { printf("%p", &f); }'
accept 'fn main() { local c: int = 1; local q: -> fn(): int = c ? &f : &f; }'
accept 'global q: -> fn(): int = &f;'
accept 'struct S { p: -> fn(): int; }; global s: S = {&f}; fn main() { (*s.p)(); }'
accept 'fn main() { local q: -> fn(): int = &f; local n: u64 = sizeof(q); }'
exit $status
