# Static analysis (lint)

Prototype of static analysis in the ABC compiler. The example analysis is a
warning for local variables that are, or may be, used uninitialized.

## Usage

By default the compiler does no static analysis. Undefined behavior, e.g. using
an uninitialized variable, stays visible as such, and static analysis is an
extra step that has to be requested:

| Option | Effect |
|--------|--------|
| `-Wall` | Enable all static analyses |
| `-Wuninitialized` | Warn about local variables that are, or may be, used uninitialized |

The generated code is the same with and without these options.

## Design

The analysis runs on the unoptimized LLVM IR of a completed function. LLVM IR
already is the control flow graph, so `&&`, `||`, `?:`, switch fallthrough and
`goto` need no special handling. The source information that the IR does not
carry is recorded during code generation in a side table.

## New files

| File | Lines | Purpose |
|------|-------|---------|
| `analysis/uninit.hpp` | 1-29 | Interface: `opt::uninitialized`, `declareLocal()`, `noteRead()`, `checkUninitialized()` |
| `analysis/uninit.cpp` | 1-228 | Side table and dataflow analysis (see below) |
| `analysis/README.md` | all | This file |
| `test/driver/static_analysis_options.sh` | all | Test T0281: no analysis by default, `-Wall` and `-Wuninitialized` enable it, generated code unchanged, unknown `-W` option rejected, options listed by `--help` |

Parts of `analysis/uninit.cpp`:

| Lines | Purpose |
|-------|---------|
| 27-31 | `opt::uninitialized`: is the analysis enabled (default: no) |
| 49-54 | Side table: declared local variables, and source location of each read |
| 56-73 | `declareLocal()` and `noteRead()` fill the side table if the analysis is enabled |
| 75-87 | `feasibleSuccessors()`: follows only the taken edge of a branch on a constant, e.g. `while (1)` or `if (0)` |
| 89-108 | `transfer()`: effect of a basic block on the state of the variables |
| 110-225 | `checkUninitialized()`: selects the variables to track, computes the fixed point, reports the first read of each variable in source order, and clears the side table |

## Modified files

In source files, modified lines are enclosed by `// for lint (begin)` and
`// for lint (end)` (`#` instead of `//` in the Makefile). The line ranges below
include these markers. Markdown files have no such markers.

| File | Lines | Purpose |
|------|-------|---------|
| `README.md` | 35 | Contents: entry for the section "Static Analysis" |
| `README.md` | 906-989 | Section "Static Analysis": why there is no static analysis by default, and what a compiler has to do for such a warning |
| `Makefile` | 74-76 | Add `analysis/*.cpp` to the compiler sources |
| `abc/abc.cpp` | 8-10 | Include `analysis/uninit.hpp` |
| `abc/abc.cpp` | 113-118 | Usage message: `-Wall` and `-Wuninitialized` |
| `abc/abc.cpp` | 236-245 | Parse `-Wall` and `-Wuninitialized`; any other `-W` option is an error |
| `ast/ast.cpp` | 5-7 | Include `analysis/uninit.hpp` |
| `ast/ast.cpp` | 344-346 | `AstFuncDef::codegen()`: run the analysis once the function is complete and unreachable blocks are removed |
| `ast/ast.cpp` | 816-821 | `AstLocalVar::codegen()`: record name and location of a local variable (declaration of a single variable) |
| `ast/ast.cpp` | 831-836 | `AstLocalVar::codegen()`: record name and location of a local variable (declaration of a variable list) |
| `expr/identifier.cpp` | 4-6 | Include `analysis/uninit.hpp` |
| `expr/identifier.cpp` | 70-74 | `Identifier::loadValue()`: record the source location of a read |

## Limitations

- Only variables whose address never escapes are tracked
  (`llvm::isAllocaPromotable`). Struct members, array elements and variables
  with `&x` are not checked, so uninitialized reads of these are not reported.
  There are no false warnings because of them either.
- A declaration does not reset the state of a variable. A variable declared in
  a loop body and read before assignment in every iteration gets "may be used"
  instead of "is used".
- The tests check the options, but not yet the warnings for the individual
  control flow cases: `test/run` and `test/fail` have no expectation for
  warnings.
