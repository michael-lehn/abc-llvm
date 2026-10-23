# Known Compiler Bugs and Open Language Design Questions

This file is the registry of known issues of the ABC compiler and of open
questions about the ABC language. Issue IDs are stable and never reused.
Tests refer to issues; `make check` validates these references.

Format of an entry:

    Issue #NNNN: Short descriptive title
    Type: BUG | DESIGN
    Status: OPEN | RESOLVED | WONTFIX
    Covered by tests: #Tnnnn (kind, CLASS), ...   (or: none)

- `kind` is the test kind: `runtime` (`test/run`), `reject` (`test/fail`),
  `design` (`test/design`) or `unit` (a `CHECK_KNOWN_BUG` in `test/unit`).
- `CLASS` is the classification of the test: `PASS`, `XFAIL` or `DESIGN`.
  - An OPEN BUG is covered by XFAIL tests that describe the expected behavior.
    Once the bug is fixed they report XPASS (a failure of `make check`). Then
    turn them into regular tests and set the issue to RESOLVED.
  - An OPEN DESIGN question is covered by DESIGN tests that record the
    currently observed behavior (ACCEPTED or REJECTED) without making it the
    language specification.

---

## Bugs

Issue #0001: Struct and array arguments passed by value are corrupted
Type: BUG
Status: RESOLVED
Covered by tests: #T0044 (runtime, PASS), #T0190 (runtime, PASS), #T0191 (runtime, PASS), #T0192 (runtime, PASS), #T0193 (runtime, PASS), #T0194 (runtime, PASS), #T0195 (runtime, PASS), #T0196 (runtime, PASS)

Aggregates without target-specific lowering are passed `byval` (on all
targets except the four aggregate shapes coerced for x86-64). The callee
initialized its parameter variable by loading and storing the ABI type of the
parameter (a pointer) instead of the aggregate type (`gen/abi.cpp`,
`reconstructArgument()`): aggregates larger than 8 bytes were only partially
initialized, smaller ones were read and written out of bounds. This affected
ABC-to-ABC calls on all architectures. Fixed: the callee copies the complete
aggregate from its private `byval` copy. The tests cover sizes below 8, of
exactly 8, of 9 to 16 and above 16 bytes, nested aggregates, several
parameters and modification of the parameter. (C interoperability for
aggregates is a separate matter: only the x86-64 shapes are lowered.)

Issue #0002: continue in a for loop skips the update expression
Type: BUG
Status: OPEN
Covered by tests: #T0017 (runtime, XFAIL)

`AstFor::codegen()` (`ast/ast.cpp`) uses the condition label as continue
target, so `for (...; ...; ++i) { continue; }` never increments `i`.
Expected: `continue` jumps to the update expression, then to the condition.

Issue #0003: pointer - integer crashes the compiler
Type: BUG
Status: OPEN
Covered by tests: #T0048 (runtime, XFAIL)

Type checking (`expr/promotion.cpp`, `binaryPtr`) accepts `p - n`, but
`BinaryExpr::handleArithmetricOperation()` (`expr/binaryexpr.cpp`) assumes
pointer - pointer and fails an assertion. Expected: `p - n` points `n`
elements before `p`.

Issue #0004: pointer -= integer is rejected
Type: BUG
Status: OPEN
Covered by tests: #T0049 (runtime, XFAIL)

`binaryPtr` in `expr/promotion.cpp` requires a pointer on the right side of
`-=`. `p += n` and `p--` are accepted. Expected: `p -= n` is equivalent to
`p = p - n`.

Issue #0005: Hexadecimal literals always have type i32 and are truncated
Type: BUG
Status: OPEN
Covered by tests: #T0033 (runtime, XFAIL), #T0156 (unit, XFAIL)

`getIntType()` in `expr/integerliteral.cpp` passes the literal including the
`0x` prefix to `std::from_chars`, which stops at the `x` and reports success
for the value 0. Thus every hexadecimal literal gets type i32 and e.g.
`0x100000000` becomes 0 without warning. Expected: hexadecimal literals get
the same type rules as decimal literals (see also #0027).

Issue #0006: Address of a global variable rejected in global initializers
Type: BUG
Status: OPEN
Covered by tests: #T0021 (runtime, XFAIL)

`global p: -> int = &x;` fails with "initializer is not a compile-time
constant". Regression of commit 352a407: `Identifier::hasConstantAddress()`
(`expr/identifier.cpp`) checks the linkage of the variable, but `global`
variables get their linkage only later. `abc-example/misc/struct.abc` and
`abc-example/misc/global_ptr.abc` no longer compile. Expected: addresses of
global and static variables are constant expressions.

Issue #0007: Array converts implicitly to a pointer to a different element type
Type: BUG
Status: OPEN
Covered by tests: #T0075 (reject, XFAIL), #T0157 (unit, XFAIL)

`convert()` in `type/type.cpp` accepts array-to-pointer conversions if the
element type merely converts to the referenced type, so
`local p: -> double = a;` with `a: array[2] of int` compiles. Expected: like
pointer-to-pointer conversion, the element type and the referenced type must
be equal (up to readonly).

Issue #0008: Small integer arguments of variadic functions are not promoted
Type: BUG
Status: OPEN
Covered by tests: #T0070 (runtime, XFAIL)

`promotion::call()` (`expr/promotion.cpp`) converts variadic arguments of
type float to double but passes i8/i16/u8/u16 values unchanged, so
`printf("%d", (i8)-1)` prints 255. Expected (C interoperability): integers
narrower than int are converted to int (sign or zero extended).

Issue #0009: Type long has 32 bits
Type: BUG
Status: OPEN
Covered by tests: #T0041 (runtime, XFAIL)

`parser/defaulttype.cpp` defines `long` with `IntegerType::createInt()`
(the unused `createLong()` exists). C's `long` has 64 bits on LP64 targets,
so C functions declared with `long` in the ABC headers (e.g. `strtol` in
`abc-include/stdio.hdr`) return truncated values. Expected: `long` matches
the C type of the target.

Issue #0010: Lexer hangs on a // comment at the end of a file without newline
Type: BUG
Status: OPEN
Covered by tests: #T0009 (runtime, XFAIL)

`getToken_()` in `lexer/lexer.cpp` skips a `//` comment until `'\n'` and
loops forever at the end of the file. Expected: a comment ends at the end of
the file.

Issue #0011: Block comment at the end of a file without newline reported as unterminated
Type: BUG
Status: OPEN
Covered by tests: #T0005 (runtime, XFAIL)

After `*/` the lexer reads the next character and reports "multi line comment
not terminated" if it is EOF. Expected: the comment is terminated.

Issue #0012: Octal escape sequences in string literals corrupt the literal
Type: BUG
Status: OPEN
Covered by tests: #T0061 (runtime, XFAIL), #T0159 (unit, XFAIL)

`parseStringLiteral()` in `lexer/lexer.cpp` stores the value of an octal
escape in `reader->ch`, overwriting the next character: `"\101B"` becomes
`"AA"`, and `"a\0"` consumes the closing quote. Expected: escapes as in C.

Issue #0013: ++ and -- are rejected for double
Type: BUG
Status: OPEN
Covered by tests: #T0014 (runtime, XFAIL)

`promotion::unary()` (`expr/promotion.cpp`) accepts increment and decrement
for integers, pointers and `float` (`isFloat()`), but not for `double`.
Expected: all floating point types (`isFloatType()`).

Issue #0014: Parenthesized expression starting with a compound literal is parsed as a cast
Type: BUG
Status: OPEN
Covered by tests: #T0046 (runtime, XFAIL)

`parsePrefix()` (`parser/parseexpr.cpp`) treats `(` followed by a type as a
cast, so `(u8{300} == 44)`, `(Point{1, 2}).y` and `assert(int{y} == 3)` are
rejected. Expected: a compound literal is a primary expression and can be
parenthesized.

Issue #0015: Column numbers after a tab are off by one
Type: BUG
Status: OPEN
Covered by tests: #T0160 (unit, XFAIL)

`nextCh()` in `lexer/reader.cpp` advances the column of a tab with
`tabStop - col % tabStop` for 1-based columns, so a token after a leading tab
starts in column 8 instead of 9. Affects diagnostics.

Issue #0016: Printing some token kinds aborts
Type: BUG
Status: OPEN
Covered by tests: none

`TokenKindCStr()` in `lexer/tokenkind.cpp` has no entries for `TRUE`, `FALSE`,
`CARET_EQUAL`, `AND_EQUAL` and `OR_EQUAL` and fails an assertion, e.g. when
`lexer/xtest_lexer` prints `a ^= b`. Not testable in-process (abort).

Issue #0017: Driver option -L with separate argument is mishandled
Type: BUG
Status: OPEN
Covered by tests: none

In `abc/abc.cpp`, `-L dir` adds `-Ldir` to the linker flags but does not skip
`dir`, which is then treated as input file. `-Ldir` works.

Issue #0018: Result of the LLVM function verifier is ignored
Type: BUG
Status: OPEN
Covered by tests: none

`functionDefinitionEnd()` (`gen/function.cpp`) calls `llvm::verifyFunction()`
without checking the result, so invalid IR is not reported by the compiler.
(The test suite verifies the IR of all runtime tests with `opt`.)

Issue #0019: End of an included file without newline continues with tokens of the including file
Type: BUG
Status: OPEN
Covered by tests: #T0161 (runtime, XFAIL)

When the reader reaches the end of an included file it continues with the
including file (`nextCh()` in `lexer/reader.cpp`) without ending the current
line. A directive in the last line, e.g. `@define N 7` without newline,
consumes tokens of the including file up to its next newline; the text of a
token at the end of the file is taken from the including file's reader.
Expected: the end of an included file ends the current token and directive.

Issue #0020: alignas and alignof are not reserved
Type: BUG
Status: OPEN
Covered by tests: #T0158 (unit, XFAIL)

The README lists `alignas` and `alignof` as reserved keywords (the operators
are documented as not yet implemented), but the lexer treats them as
identifiers. Expected: they cannot be used as identifiers.

Issue #0021: Documented type names long_long, unsigned_long, unsigned_long_long are undefined
Type: BUG
Status: OPEN
Covered by tests: #T0013 (runtime, XFAIL)

The README lists them as predefined type names; they existed as type aliases
(commit 4038f15, February 2024) but are missing in `parser/defaulttype.cpp`.
Expected: defined with the sizes of the corresponding C types (see #0009).

Issue #0022: Lexer diagnostics show the location of the previous token
Type: BUG
Status: OPEN
Covered by tests: none

Errors detected while scanning a token (invalid octal/hexadecimal literal,
unterminated string or comment, ...) are reported at `token.loc`, the location
of the previous token (`:1.1-1.1` without file name at the start of a file).

Issue #0032: Underlying type of an enum definition ignored after a forward declaration
Type: BUG
Status: OPEN
Covered by tests: #T0091 (reject, XFAIL)

After `enum F : u8;` the definition `enum F : u16 { A };` is accepted and
`F` keeps type u8; after `enum F;` (type int) the definition
`enum F : u8 { ... }` silently keeps int. A redefinition of the constants is
rejected as incompatible. Expected: a definition whose type differs from the
forward declaration is an incompatible redefinition.

Issue #0034: Later optimization option did not reset -Os or -Oz
Type: BUG
Status: RESOLVED
Covered by tests: #T0169 (ir, PASS), #T0170 (ir, PASS), #T0171 (ir, PASS), #T0172 (ir, PASS), #T0173 (ir, PASS), #T0174 (ir, PASS), #T0175 (ir, PASS), #T0176 (ir, PASS), #T0177 (ir, PASS), #T0178 (ir, PASS), #T0179 (ir, PASS), #T0180 (ir, PASS), #T0181 (ir, PASS), #T0182 (ir, PASS), #T0183 (ir, PASS), #T0184 (ir, PASS), #T0185 (ir, PASS), #T0186 (ir, PASS), #T0187 (ir, PASS)

With the LLVM 23 changes, -Os and -Oz select -O2 plus the function attributes
`optsize` (and `minsize`) via `gen::opt::sizeLevel` (`abc/abc.cpp`). A later
-O0, -O1, -O2 or -O3 did not reset the size level, so `-Os -O2` still emitted
`optsize`. Fixed before the change was committed: the last optimization
option takes precedence. The tests in test/ir compile with option
combinations and check the attributes and the optimization of the generated
IR.

Issue #0036: LLVM miscompile of by-value arguments in tail-recursive functions (LLVM #235074, worked around)
Type: BUG
Status: RESOLVED
Covered by tests: #T0196 (runtime, PASS), #T0209 (runtime, PASS)

An LLVM optimizer bug (LLVM 17 to at least 24): `tailcallelim` turns a
self-recursive call with a `byval` argument into a loop that writes to the
function's own `byval` argument, but keeps the function attribute
`memory(argmem: read)`; the inliner then elides the `byval` copy, so the
caller's object is modified. ABC generates valid IR; the bug is reported
upstream as https://github.com/llvm/llvm-project/issues/235074 (reproducer:
doc/llvm-issues/tailcallelim-byval). Workaround in ABC: calls with `byval`
arguments are marked `notail` (`gen/abi.cpp`, `lowerFunctionCall`). T0209
observes the caller's object through C and fails without the workaround on
LLVM 17 to 23; T0196 fails without it on LLVM 17 to 19.

---

## Open Language Design Questions

Issue #0023: Purpose of the punctuator #
Type: DESIGN
Status: OPEN
Covered by tests: none

The README lists `#` as punctuator and the lexer has a token kind `HASH`, but
the lexer never produces it and no grammar rule uses it. Question: is `#`
reserved for a future feature, or should it be removed from the list?

Issue #0024: Function-style cast int(y)
Type: DESIGN
Status: OPEN
Covered by tests: #T0144 (design, DESIGN)

The README documents casts as `int(y)` and states that the C cast syntax is
not supported. The compiler implements the C-style cast `(T)expr` and the
compound literal `T{expr}`, which are used throughout
`abc-example/misc` (e.g. `cast.abc`), and rejects `int(y)`. Question: which
cast syntax(es) does ABC have? Note that `T(expr)` is ambiguous for types
that are not identifiers (e.g. `-> int(p)`).

Issue #0025: Unary plus
Type: DESIGN
Status: OPEN
Covered by tests: #T0151 (design, DESIGN)

The README precedence table lists unary `+`; the README expression grammar
and the compiler do not have it. Question: should ABC have unary plus?

Issue #0026: Mixing ? with else and then with : in conditional expressions
Type: DESIGN
Status: OPEN
Covered by tests: #T0145 (design, DESIGN), #T0146 (design, DESIGN)

The README states that `?`/`then` and `:`/`else` are interchangeable
("at least currently") and gives `a > b ? y else z` as example. The parser
(commit 07f317b, after that README text) requires `?` with `:` and `then`
with `else`. Question: are mixed forms part of the language?

Issue #0027: Type and range of integer literals
Type: DESIGN
Status: OPEN
Covered by tests: #T0147 (design, DESIGN)

README: decimal literals are unsigned and can be of arbitrary size.
Compiler: an unsuffixed literal has type i32 if it fits, otherwise i64; larger
values only produce a warning. A TODO in `expr/integerliteral.cpp` mentions
Rust-like literal typing. Literals with type suffix (`255u8`) are
undocumented. Question: what are the type and the valid range of literals?

Issue #0028: Arithmetic with mixed signedness and without integer promotion
Type: DESIGN
Status: OPEN
Covered by tests: #T0148 (design, DESIGN)

The README does not define the usual arithmetic conversions. The compiler
uses "unsigned wins, size is the maximum" (`Type::common()`) and has no
integer promotion, so `(i64)-1 < (u8)1` is false and `true + true` is 0.
Question: which conversion rules does ABC have?

Issue #0029: Signedness of size_t and ptrdiff_t
Type: DESIGN
Status: OPEN
Covered by tests: #T0150 (design, DESIGN)

Both are signed 64-bit types (`IntegerType::createSizeType()`); in C,
`size_t` is unsigned. Question: is a signed `size_t` intended?

Issue #0030: Undocumented type names short, ushort and sshort
Type: DESIGN
Status: OPEN
Covered by tests: #T0149 (design, DESIGN)

`parser/defaulttype.cpp` defines `short`, `ushort` and `sshort` ("signed
short") as unsigned 32-bit types. They are not documented. Question: should
they exist, and with which size and signedness?

Issue #0031: case labels with an expression list
Type: DESIGN
Status: OPEN
Covered by tests: #T0143 (design, DESIGN)

The README grammar allows `"case" expression-list ":"`. The compiler
evaluates the list with the comma operator, so `case 3, 4:` matches only 4.
Question: should `case a, b:` match several values, or be an error?

Issue #0033: Return-path analysis and constant loop conditions
Type: DESIGN
Status: OPEN
Covered by tests: #T0168 (design, DESIGN)

`functionDefinitionEnd()` (`gen/function.cpp`) reports "non-void function
does not return a value in all control paths" if the end of the function is
structurally reachable. A loop without condition (`for (;;)`) has no exit,
but the condition of `while (true)` is not folded (`br i1 true`), so a
non-void function ending with `while (true) { ... }` is rejected while the
equivalent `for (;;)` loop is accepted. Question: which control-flow analysis
does the language require (structural, or with constant conditions folded)?
