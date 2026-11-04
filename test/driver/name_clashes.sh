# TEST: T0332
# Names defined twice get an ordinary error, not a crash, and what is allowed stays
# valid: other scopes, and names of parameters in a declaration.

status=0
compile()
{
    echo "$1" > t.abc
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

reject 'fn f(a: int, a: int) { }'
reject 'fn f(a: int, a: double) { }'
reject 'fn f(a: int, b: int, a: int): int { return a; }'
reject 'fn f(a, a: int) { }'
accept 'fn f(a: int, a: int);'
accept 'fn f(: int, : int) { }'
accept 'fn f(a: int, b: int) { local c: int; }'

reject 'enum E { f }; fn f() { }'
reject 'enum E { f }; fn f();'
reject 'enum E { f }; global f: int;'
reject 'enum E { f }; extern f: int;'
reject 'enum E { f }; type f: int;'
reject 'enum E { f }; enum G { f };'
reject 'global f: int; enum E { f };'
reject 'fn f() { } enum E { f };'
reject 'fn main() { enum E { f }; local f: int; }'
accept 'enum E { f }; fn main() { local f: int; }'
accept 'enum E { f }; fn g(f: int) { }'
exit $status
