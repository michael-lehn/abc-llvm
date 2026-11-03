# TEST: T0327
# Programs that have to be rejected with an ordinary error next to similar ones
# that stay valid: conversion of arrays to pointers, type suffixes of literals,
# declarations of enums, reserved words.

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

# an array converts to a pointer to its element type
a='local a: array[2] of int;'
aa='local aa: array[2] of array[3] of int;'
reject "fn main() { $a local p: -> double = a; }"
reject "fn main() { $a local p: -> i8 = a; }"
reject "fn main() { $a local p: -> u32 = a; }"
reject "fn main() { $a local p: -> -> int = a; }"
reject "fn f(p: -> double) {} fn main() { $a f(a); }"
reject "fn main() { $a local p: -> double; p = a; }"
reject "global g: array[2] of int; global p: -> double = g;"
reject "fn main() { local s: -> int = \"x\"; }"
reject "fn main() { $aa local p: -> array[4] of int = aa; }"
reject "fn main() { $aa local p: -> int = aa; }"
reject "fn main() { local r: readonly array[2] of int; local p: -> int = r; }"
accept "fn main() { $a local p: -> int = a; }"
accept "fn main() { $a local p: -> readonly int = a; }"
accept "fn main() { $a local p: -> void = a; }"
accept "fn main() { $a local p: -> readonly void = a; }"
accept "fn f(p: -> int) {} fn main() { $a f(a); }"
accept "fn main() { $aa local p: -> array[3] of int = aa; }"
accept "fn main() { local s: -> readonly char = \"x\"; }"
accept "fn main() { local r: readonly array[2] of int; local p: -> readonly int = r; }"
accept "type T: int; fn main() { local t: array[2] of T; local p: -> int = t; }"
accept "global g: array[2] of int; global p: -> int = g;"
# an explicit cast can change the type
accept "fn main() { $a local p: -> double = (-> double) a; }"
accept "fn main() { local p: -> int = (-> int) \"s\"; }"
accept "fn main() { $aa local p: -> int = (-> int) aa; }"
accept "global g: array[2] of int; global p: -> double = (-> double) g;"

# a type after a literal is a type suffix
reject 'fn main() { local x: int; x = 1 -> int; }'
reject 'fn main() { local x: double; x = 1.5 -> int; }'
reject 'struct S { a: int; }; fn main() { local x: int; x = 1 S; }'
reject 'fn main() { local x: int; x = 1 array[2] of int; }'
accept 'fn main() { local x: u8 = 255u8; local y: i64 = 1i64; local z: i64 = 1 i64; }'
accept 'type T: u8; fn main() { local x: u8 = 1 T; }'
accept 'fn main() { local d: double = 1 double; local f: float = 1 float; }'

# the type of an enum
reject 'enum F: u8; enum F: u16 { A };'
reject 'enum F: u8; enum F: i8 { A };'
reject 'enum F: u8; enum F: u16;'
reject 'enum F; enum F: u8 { A };'
reject 'enum F: u8; enum F { A };'
reject 'enum F { A }; enum F { B };'
accept 'enum F: u8; enum F: u8 { A }; global x: F = A;'
accept 'enum F; enum F { A };'
accept 'enum F; enum F: int { A };'
accept 'enum F: int; enum F { A };'
accept 'enum F: u8; enum F: u8;'
accept 'type U: u8; enum F: u8; enum F: U { A };'

# reserved words
reject 'fn main() { local alignas: int; }'
reject 'fn main() { local alignof: int; }'
reject 'global alignof: int;'
reject 'fn alignas() {}'
reject 'struct alignof { a: int; };'
reject 'struct S { alignas: int; };'
reject 'fn main() { alignof(int); }'
reject 'fn main() { local x: int = alignas; }'
accept 'fn main() { local alignment: int; local my_alignof: int; }'
exit $status
