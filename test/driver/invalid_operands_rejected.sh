# TEST: T0316
# Variants of invalid operands: each program gets an ordinary error, not a crash
# and not an internal compiler error.

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

f='fn f() {} fn g() {}'
p='local p: -> void = nullptr; local q: -> void = nullptr;'

reject "$f fn main() { if (g()) { } }"
reject "$f fn main() { while (f()) { } }"
reject "$f fn main() { local x: int = f() ? 1 : 2; }"
reject "$f fn main() { local b: bool = f() && 1; }"
reject "$f fn main() { local x: int = f() + 1; }"
reject "$f fn main() { local x: int = !f(); }"
reject "$f fn main() { local c: int = 1; if (c ? f() : g()) { } }"
reject "$f fn main() { local c: int = 1; local x: int = c ? f() : g(); }"
reject "$f fn main() { local c: int = 1; local x: int = (c ? f() : g()) + 1; }"
reject "$f extern fn printf(s: -> readonly char, ...): int; fn main() { local c: int = 1; printf(\"\", c ? f() : g()); }"
reject "fn main() { $p p++; }"
reject "fn main() { $p p--; }"
reject "fn main() { $p --p; }"
reject "fn main() { $p p += 1; }"
reject "fn main() { $p p -= 1; }"
reject "fn main() { $p p = p - 1; }"
reject "fn main() { $p p = 1 + p; }"
reject "fn main() { local d: i64 = 0; $p d = p - q; }"
reject "$f fn main() { local x: u64 = sizeof(f()); }"
reject "fn main() { local x: u64 = sizeof(-> void) + sizeof(void); }"
reject "type V: void; fn main() { local x: u64 = sizeof(V); }"
reject "fn main() { local p: -> int = 1 + nullptr; }"
reject "fn main() { local p: -> int = nullptr - 1; }"
reject "fn main() { local d: i64 = nullptr - nullptr; }"
reject "fn main() { local p: -> int = nullptr; local d: i64 = p - nullptr; }"
reject "fn main() { local x: double = 1; switch (x) { default: break; } }"
reject "fn main() { local p: -> int = nullptr; switch (p) { default: break; } }"
reject "struct S { a: int; }; fn main() { local s: S; switch (s) { default: break; } }"
reject "$f fn main() { switch (f()) { default: break; } }"
reject "global x: int; enum E { A = x };"
reject "enum E { A = \"s\" };"
reject "global a: int; global z: int; global b: bool = &a != &z;"
reject "global a: int; global z: int; global b: bool = &a < &z;"
reject "global a: int; global b: bool = &a == nullptr;"
reject "global a: int; global z: int; global d: i64 = &a - &z;"
exit $status
