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
  `design` (`test/design`), `ir` (`test/ir`), `driver` (`test/driver`) or
  `unit` (a `CHECK_KNOWN_BUG` in `test/unit`).
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
Covered by tests: #T0044 (runtime, PASS), #T0190 (runtime, PASS), #T0191 (runtime, PASS), #T0192 (runtime, PASS), #T0193 (runtime, PASS), #T0194 (runtime, PASS), #T0195 (runtime, PASS), #T0196 (runtime, PASS), #T0202 (ir, PASS), #T0203 (ir, PASS)

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
Status: RESOLVED
Covered by tests: #T0206 (runtime, PASS), #T0207 (ir, PASS), #T0208 (reject, PASS)

In `abc/abc.cpp`, `-L dir` added `-Ldir` to the linker flags but did not skip
`dir`, which was then treated as input file (the linker failed). `-Ldir`
worked. Fixed like `-target` (#0035): the separate argument is consumed; a
missing directory is reported (usage, exit status 1) like for -o, -I and -l.

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

Issue #0035: Option -target did not consume its argument
Type: BUG
Status: RESOLVED
Covered by tests: #T0197 (ir, PASS), #T0198 (ir, PASS), #T0199 (ir, PASS), #T0200 (ir, PASS), #T0201 (ir, PASS), #T0204 (reject, PASS), #T0205 (reject, PASS)

The parser (`abc/abc.cpp`) stored the triple following `-target` but did
not skip it, so the triple was also treated as an input file (e.g. "cannot
specify --emit-llvm when generating multiple output files"). Fixed: `-target`
consumes exactly one argument; a missing triple and an unknown triple are
reported with a diagnostic and exit status 1. The tests check the triple and
data layout of the generated IR for x86-64 and AArch64, the position of
`-target` relative to the input file and other options, and both errors.
Generating IR for a target does not imply C ABI support for that target.

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

Issue #0037: A readonly array of arrays crashes the compiler
Type: BUG
Status: OPEN
Covered by tests: #T0210 (runtime, XFAIL)

Declarations like `readonly array[2] of array[1] of int` crash the compiler
(stack overflow). The comparison
of the array type set (`operator<` in `type/arraytype.cpp`) calls the
virtual `refType()`, which for a readonly array creates the readonly element
type with `ArrayType::create()` and thus inserts into the set that is being
searched; this recurses endlessly. Whether the recursion is reached depends
on the order of addresses in the heap: with builds against LLVM 21 to 23 two
declarations crash, with builds against LLVM 17 to 20 more than 16 are
needed; the test uses 64 (50 of 50 runs crash with every build). (The
comparison also uses `x.dim()` for both operands.) Expected: the program
compiles and runs.

Issue #0038: Shift operands of different width produce invalid LLVM IR
Type: BUG
Status: OPEN
Covered by tests: #T0211 (runtime, XFAIL), #T0212 (runtime, XFAIL)

For `x << n`, `x >> n`, `x <<= n` and `x >>= n` the shift count keeps its own
type (`expr/promotion.cpp`), so operands of different width produce invalid
IR, e.g. `shl i64 %0, i32 3` for `x << 3` with `x: u64` (the IR cannot even be
parsed by LLVM). Every combination of different widths is affected, e.g. a
64-bit or 8-bit value shifted by an `int` literal; equal widths are fine. A
compound shift assignment also stores with the alignment of the right
operand's type (`store i8 ..., align 8` for `c: i8` and `n: i64`). The
compiler does not notice because of #0018. Expected: the shift count is
converted to the type of the left operand; the IR is valid.

Issue #0039: A floating-point value as condition crashes the compiler
Type: BUG
Status: OPEN
Covered by tests: #T0213 (runtime, XFAIL)

`if (d)`, `while (d)` and `d ? x : y` with `d` of type `double` or `float`
abort with an LLVM assertion: the value is compared with zero using an
integer comparison (`gen::NE`, `icmp`) in `Expr::condition()`
(`expr/expr.cpp`) and `ConditionalExpr` (`expr/conditionalexpr.cpp`).
`Expr::condition()` accepts every scalar type, including floating-point
types; `!d`, `d && x` and `(bool)d` are rejected with a diagnostic. Expected
(as in C): zero is false, every other value, including NaN, is true.

Issue #0040: switch over an alias of an enum type crashes the compiler
Type: BUG
Status: OPEN
Covered by tests: #T0214 (runtime, XFAIL)

The check for unhandled enumeration values in `AstSwitch`
(`ast/ast.cpp`) casts the type of the switch expression with
`dynamic_cast<const EnumType *>`; for an alias (`type MyColor: Color;`)
`isEnum()` is true but the cast yields null and the assertion fails.
Expected: the alias is resolved and the switch is checked like one over the
enum type.

Issue #0041: static with several variables and without initializer crashes
Type: BUG
Status: OPEN
Covered by tests: #T0215 (runtime, XFAIL)

`static a, b: int;` in a function aborts with an assertion in
`AstStaticVar::codegen()` (`ast/ast.cpp`), which assumes an initializer for
several variables; `static a: int;` works, and `AstGlobalVar::codegen()`
handles the case. Expected: the variables are zero-initialized.

Issue #0042: Hexadecimal literal with prefix 0X crashes the compiler
Type: BUG
Status: OPEN
Covered by tests: #T0216 (runtime, XFAIL)

`0XFF` aborts with the assertion `val[1] == 'x' || val[1] == 'Y'` in
`gen::getConstantInt()` (`gen/constant.cpp`; typo `'Y'` for `'X'`). The lexer
and the README accept the prefix `0X`. (#0005 concerns the type of
hexadecimal literals.) Expected: `0XFF == 255`.

Issue #0043: Cast of a readonly struct to the struct type crashes the compiler
Type: BUG
Status: OPEN
Covered by tests: #T0217 (runtime, XFAIL)

`(P)rp` with `rp: readonly P`, used as a by-value argument or for member
access, crashes the compiler (segmentation fault). `ExplicitCast::hasAddress()`
returns true, but `ExplicitCast::loadAddress()` (`expr/explicitcast.cpp`)
returns a null pointer after storing the value in a temporary
(`ImplicitCast::loadAddress()` returns the temporary). Expected: the cast
yields the struct value.

Issue #0044: By-value array parameters with 2 to 4 elements crash the compiler on x86-64
Type: BUG
Status: OPEN
Covered by tests: #T0218 (ir, XFAIL)

On x86-64 `fn f(a: array[2] of int)` or `fn f(a: array[4] of u8)` aborts with
the assertion `abcType->isStruct()` in `isHomogeneousAggregate()`
(`gen/abi.cpp`): for an array whose element type is not the one checked for,
the function falls through to the struct case. Other targets are not
affected. Expected: such arrays are passed according to the x86-64 ABI.

Issue #0045: --print-ast crashes after an initializer overrides another
Type: BUG
Status: OPEN
Covered by tests: #T0219 (driver, XFAIL)

`{[0] = 1, [0] = 2}` produces the warning "initializer overrides prior
initialization"; `--print-ast` and `-E` then crash (segmentation fault;
AddressSanitizer: heap-use-after-free). `CompoundExpr::create()`
(`expr/compoundexpr.cpp`) replaces the first expression but keeps a pointer
to it in `parsedExpr`, which `printFlat()` uses. Expected: the AST is printed
as written.

Issue #0046: Positional initializer after a designator is converted to the wrong type
Type: BUG
Status: OPEN
Covered by tests: #T0220 (runtime, XFAIL)

In `{.b = 1.5, 7}` for `struct S { a: i8; b: double; c: i32; }` the value
`7` initializes `c` but is converted to the type of the second member
(`double`): `CompoundExpr::create()` (`expr/compoundexpr.cpp`) takes the
type from the position in the initializer list (`aggregateType(i)`) instead
of the initialized member. `s.c` gets a wrong value. Expected: `s.c == 7`.

Issue #0047: Compound assignment, ++ and -- evaluate their operand twice
Type: BUG
Status: OPEN
Covered by tests: #T0221 (runtime, XFAIL), #T0222 (runtime, XFAIL)

`a[i++] += 10` reads `a[0]`, writes `a[1]` and increments `i` twice;
`a[i++]++`, `++a[i++]` and `*p++ += 10` behave alike. The operand is
evaluated once for its value and once more for its address
(`BinaryExpr::loadValue()` in `expr/binaryexpr.cpp`, `UnaryExpr::loadValue()`
in `expr/unaryexpr.cpp`). Simple assignment is not affected. The README does
not specify compound assignment; expected (as in C): the operand is
evaluated once.

Issue #0048: -> on a pointer expression without address is rejected
Type: BUG
Status: OPEN
Covered by tests: #T0223 (runtime, XFAIL)

`(p + 1)->x` is rejected with "p + 1 has no address (internal compiler
error)": `Member::create()` (`expr/member.cpp`) requires an address of the
operand also for `->`, where only its value is needed. Expected: `->` works
for every pointer expression.

Issue #0049: Coerced x86-64 aggregate arguments are accessed with too high alignment
Type: BUG
Status: OPEN
Covered by tests: #T0224 (ir, XFAIL)

On x86-64 a struct of two floats is passed as `<2 x float>`; the argument is
loaded (and the parameter stored) with the alignment of `<2 x float>` (8)
instead of that of the struct (4), e.g. `load <2 x float>, ptr @gv, align 8`
for a global (`lowerArgument()` and `reconstructArgument()` in
`gen/abi.cpp`). Claiming a higher alignment than the memory has is undefined
behavior in LLVM IR; no wrong result has been observed. Expected: the access
uses the alignment of the aggregate.

Issue #0050: Printed expressions lose necessary parentheses
Type: BUG
Status: OPEN
Covered by tests: #T0225 (ir, XFAIL)

`printFlat()` (e.g. `BinaryExpr::printFlat()` in `expr/binaryexpr.cpp`)
prints both operands with the precedence of the operator, so `a - (b - c)`
is printed as `a - b - c`. This affects the message of a failed assertion
(`Assertion 'a - b - c == 0' failed`), diagnostics and `--print-ast`.
Expected: parentheses are kept where they are needed for the meaning.

Issue #0051: switch over a readonly enum does not report unhandled values
Type: BUG
Status: OPEN
Covered by tests: #T0226 (reject, XFAIL)

A switch without default over a value of type `readonly Color` that does not
handle all enumeration values is accepted; for type `Color` it is rejected
("enumeration values ... not handled in switch"). The readonly variant of an
enum type has no enumeration constants (`EnumType::complete()` in
`type/enumtype.cpp` only marks it complete). Expected: the same error as for
the non-readonly type.

Issue #0052: Non-arithmetic type after a literal is silently discarded
Type: BUG
Status: OPEN
Covered by tests: #T0227 (reject, XFAIL)

`parseSuffixType()` (`parser/parseexpr.cpp`) parses a type after a literal
as type suffix and returns no type if it is not arithmetic, after consuming
its tokens: `x = 1 -> int;` compiles as `x = 1;`. Expected: an error.

Issue #0053: Unknown long option exits with status 0
Type: BUG
Status: OPEN
Covered by tests: #T0228 (reject, XFAIL)

An unknown option starting with `--` prints the usage and exits with status
0 (`abc/abc.cpp`), so build tools continue; an unknown short option exits
with status 1. Expected: an error message and a nonzero exit status.

Issue #0054: @define followed by a // comment takes the next line into the macro
Type: BUG
Status: OPEN
Covered by tests: #T0229 (runtime, XFAIL)

`@define N 10 // comment` ends the directive only at the end of the next
line: after a comment `getToken_()` (`lexer/lexer.cpp`) calls `getToken()`,
which skips the newline that ends the directive. Expected: the comment ends
at the end of its line and the directive with it.

Issue #0055: @define in the last line of a file without newline hangs the compiler
Type: BUG
Status: OPEN
Covered by tests: #T0230 (runtime, XFAIL)

The loop that reads the replacement of a macro (`lexer/lexer.cpp`) stops
only at a newline token, not at the end of the input. (#0019 concerns the
same situation in an included file, which does not hang.) Expected: the end
of the file ends the directive.

Issue #0056: @ <file without closing bracket hangs the compiler
Type: BUG
Status: OPEN
Covered by tests: #T0231 (reject, XFAIL)

The lexer reads the name of an `@ <...>` include up to the next `>`
(`lexer/lexer.cpp`) without checking for the end of the line or the input;
without `>` it loops forever at the end of the file. Expected: an error.

Issue #0057: @ <file> include is searched in a disabled @ifdef region
Type: BUG
Status: OPEN
Covered by tests: #T0232 (runtime, XFAIL)

Inside a disabled `@ifdef` region an `@ <file>` include is still searched and
reported as "can not find file" (`lexer/lexer.cpp`); an `@ "file"` include is
skipped there. Expected: both forms are skipped.

Issue #0058: Negative designator for an array of unbound size hangs the compiler
Type: BUG
Status: OPEN
Covered by tests: #T0233 (reject, XFAIL)

`local a: array[] of int = {[-1] = 1};` hangs: the designator is converted
with `getUnsignedIntValue()` (`parser/parseexpr.cpp`) and the bound check is
skipped for an unbound array, so the index wraps around. For an array with
a size the designator is rejected. Expected: an error.

Issue #0059: Several input files for x86-64 crash the compiler (freed type)
Type: BUG
Status: OPEN
Covered by tests: #T0234 (driver, XFAIL)

`lowerType_x86_64()` (`gen/abi.cpp`) keeps pointers to interned types in
`static` variables. The driver resets the type system for each input file
(`abc/abc.cpp`, `initTypeSystem()`), which frees them; classifying a
4-byte aggregate in a later file then uses freed memory (AddressSanitizer:
heap-use-after-free). Depending on the heap, the compiler crashes or
silently miscompiles: a struct of four `u8` is passed in memory (`byval`)
instead of in a register, which breaks the ABI. The test compiles four such
files after another one (no run of 350 was correct with any build).
Expected: several files can be compiled in one invocation, each with the
same result as alone.

Issue #0060: Input files with the same name overwrite each other's object file
Type: BUG
Status: OPEN
Covered by tests: #T0235 (driver, XFAIL)

When linking, each input file is compiled to `<temporary directory>/<name>.o`
(`abc/abc.cpp`), so `d1/same.abc` and `d2/same.abc` use the same object file
and the first is lost (link error). The temporary files are not removed
either. Expected: unique temporary files, removed after linking.

Issue #0061: With -c an input file that is not ABC source is compiled as ABC
Type: BUG
Status: OPEN
Covered by tests: #T0236 (driver, XFAIL)

`abc -c ok.abc other.c` warns that `other.c` (a linker input) is unused and
then parses it as ABC source (`abc/abc.cpp`), which fails. Expected: the file
is skipped after the warning.

Issue #0062: Error at the end of the second input file is reported in the first
Type: BUG
Status: OPEN
Covered by tests: #T0237 (driver, XFAIL)

With several input files the lexer keeps the exhausted reader of the
previous file on its stack (`openInputfile()` in `lexer/reader.cpp`); at the
end of the next file it continues there, so an error at the end of the
second file is reported with the name and position of the first. Expected:
the error is reported for the second file.

Issue #0063: Dependency files are wrong with several input files
Type: BUG
Status: OPEN
Covered by tests: #T0238 (driver, XFAIL)

With `-MD` and several input files only one dependency file is written,
named after the first file and with its target, but with the dependencies of
the last file (`abc/abc.cpp` sets the file name and target only once).
Expected: one dependency file per input file.

Issue #0064: Diagnostics show no source line for read-only files
Type: BUG
Status: OPEN
Covered by tests: #T0239 (driver, XFAIL)

The source line of a diagnostic is read with a `std::fstream` opened for
reading and writing (`lexer/error.cpp`), which fails for files without write
permission; the diagnostic then shows an empty line above the markers.
Expected: the line is shown.

Issue #0065: -mmcu=cortex-... selects an unknown processor
Type: BUG
Status: OPEN
Covered by tests: #T0240 (driver, XFAIL)

`getCpu()` (`gen/gen.cpp`) strips the prefix `cortex-` (`cortex-m3` becomes
`m3`), which LLVM does not know: it warns "'m3' is not a recognized processor
for this target (ignoring processor)" and generates code for a generic
processor. Expected: the processor `cortex-m3`.

Issue #0066: --print-ast of a struct with a union reads out of bounds
Type: BUG
Status: OPEN
Covered by tests: none

`AstStructDecl::print()` (`ast/ast.cpp`) computes whether a member list is
the last one before advancing the position over several member names, then
reads `memberIndex[pos]` with `pos == memberIndex.size()` for a union whose
last member list has several names (e.g. `union { a: int; b, c: float; };`).
Reproduced only with AddressSanitizer (heap-buffer-overflow); the output of
a normal build is correct. Expected: no out-of-bounds read.

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

Issue #0067: Evaluation order of operands with side effects
Type: DESIGN
Status: OPEN
Covered by tests: #T0241 (design, DESIGN)

The language does not specify in which order the operands of an expression
are evaluated. Observed with a compiler built by clang++: operands of binary
operators and call arguments from left to right, but for an assignment the
right operand before the left (`arr[f()] = g()` calls `g` first). The order
of binary operators and assignments results from the evaluation order of
C++ function arguments in the compiler (e.g. `gen::store(right->loadValue(),
left->loadAddress(), ...)` in `expr/binaryexpr.cpp`), which C++ leaves
unspecified, so a compiler built with another C++ compiler may differ (not
reproduced: GCC cannot link against the LLVM installation used for testing).
Call arguments are evaluated in an explicit loop. The design test prints the
observed order and does not check it. Open: specify the order (e.g. left to
right, as in Java) or leave it unspecified (as in C).
