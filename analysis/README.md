# Static analysis (lint)

Static analysis in the ABC compiler. Most of the code is in this directory,
separated from the compiler. The locations in the compiler that enable it are
marked and listed below.

## Usage

By default the compiler does no static analysis. Undefined behavior, e.g. using
an uninitialized variable, stays visible as such, and static analysis is an
extra step that has to be requested:

| Option | Effect |
|--------|--------|
| `-Wall` | Enable all static analyses, with the strict variant of `-Wnull-dereference` |
| `-Wuninitialized` | Warn about local variables that are, or may be, used uninitialized |
| `-Wnull-dereference` | Warn about local pointer variables that are, or may be, null or freed when dereferenced, and about pointers freed twice |
| `-Wnull-dereference=strict` | Also check members of structs, where an unknown member may be null, tests kept in a variable, and tests after a dereference |

The generated code is the same with and without these options.

[examples/](examples/README.md) has a small program for each kind of warning.

## Design

The analyses run on the unoptimized LLVM IR of a completed function. LLVM IR
already is the control flow graph, so `&&`, `||`, `?:`, switch fallthrough and
`goto` need no special handling. The source information that the IR does not
carry is recorded during code generation in a side table.

To add an analysis, write a function that gets the IR and the `SourceInfo`, add
a flag to `opt`, call the function in `run()` and parse the option in
`abc/abc.cpp`.

`-Wnull-dereference` follows `examples/pointers` of
[astl-c](https://github.com/afborchert/astl-c): the set of values a pointer
variable can have (null, not null, freed) is tracked along the control flow
graph. The result of `malloc`, `calloc` and `realloc` can be null or not. A test
against null restricts the set on the edges of the branch, whichever way it is
written. A value nothing is known about, e.g. a parameter, gets no warning, but
a test tells which value it has on each edge. Unlike in astl-c, the code after
`for (; n; n = n->next)` therefore stays reachable for the analysis. `exit`, `abort`, `_Exit` and `__assert` end a path, as they do not
return but are not marked like that in the IR.

`-Wnull-dereference=strict` is a separate implementation, so that the simpler
one can be read first. It adds access paths: the state belongs to `list`,
`list->next` and so on instead of a variable, and a path gets forgotten if it can
have changed by a store through another pointer or a call of an unknown function.
It also knows facts like "if `ok` is true then `p` is not null". The assumption
that an unknown member may be null is what makes it strict.

A dereference of a parameter gets no warning, as nothing is known about it. A
later test against null shows that it can be null, so the strict analysis warns
about the test: `if (list->next && list)`. It only looks at what happened before
the test, so a pointer that is never tested still gets no warning.

## New files

| File | Lines | Purpose |
|------|-------|---------|
| `analysis/analysis.hpp` | 1-36 | Interface for the compiler: `opt`, `declareLocal()`, `noteRead()`, `run()` |
| `analysis/analysis.cpp` | 1-102 | Options, side table, and `run()` that calls the enabled analyses |
| `analysis/sourceinfo.hpp` | 1-40 | `SourceInfo`: what an analysis gets to know besides the IR |
| `analysis/uninit.hpp` | 1-18 | Interface of `-Wuninitialized` |
| `analysis/uninit.cpp` | 1-175 | Dataflow analysis of `-Wuninitialized` (see below) |
| `analysis/nullderef.hpp` | 1-18 | Interface of `-Wnull-dereference` |
| `analysis/nullderef.cpp` | 1-340 | Dataflow analysis of `-Wnull-dereference` (see below) |
| `analysis/nullderef-v2.hpp` | 1-19 | Interface of `-Wnull-dereference=strict` |
| `analysis/nullderef-v2.cpp` | 1-587 | Dataflow analysis of `-Wnull-dereference=strict` (see below) |
| `analysis/README.md` | all | This file |
| `analysis/examples/` | all | One small program for each kind of warning, with a comment saying which options generate it |
| `test/driver/analysis_examples.sh` | all | Test T0289: the comments in `analysis/examples` are correct |
| `test/driver/static_analysis_options.sh` | all | Test T0281: no analysis by default, `-Wall` and `-Wuninitialized` enable it, generated code unchanged, unknown `-W` option rejected, options listed by `--help` |
| `test/driver/null_dereference.sh` | all | Test T0287: warnings of `-Wnull-dereference` for each kind of test and access |
| `test/driver/null_dereference_strict.sh` | all | Test T0288: warnings of `-Wnull-dereference=strict`, and that `-Wall` enables it |

Parts of `analysis/uninit.cpp`:

| Lines | Purpose |
|-------|---------|
| 34-46 | `feasibleSuccessors()`: follows only the taken edge of a branch on a constant, e.g. `while (1)` or `if (0)` |
| 48-67 | `transfer()`: effect of a basic block on the state of the variables |
| 69-172 | `checkUninitialized()`: selects the variables to track, computes the fixed point, and reports the first read of each variable in source order |

Parts of `analysis/nullderef.cpp`:

| Lines | Purpose |
|-------|---------|
| 66-92 | `calledName()`, `stateOfValue()`: which values can be null, by the name of the called function |
| 94-167 | `transfer()`: effect of a basic block, with the checks of dereferences and calls of `free` |
| 169-224 | `refine()`: what the condition of a branch tells about a variable on each edge |
| 226-337 | `checkNullDereference()`: selects the variables to track, computes the fixed point, and reports |

Parts of `analysis/nullderef-v2.cpp`:

| Lines | Purpose |
|-------|---------|
| 114-164 | `get()`, `forget()`, `forgetMemory()`: the state of an access path, and when it gets forgotten |
| 166-219 | `named()`, `stateOfValue()`, `onlyDirectAccess()`: which values can be null, and which variables can be the root of a path |
| 235-401 | `transfer()`: effect of a basic block, with paths of SSA values, facts of tests and the checks |
| 403-451 | `refine()`: what the condition of a branch tells about a path on each edge |
| 453-482 | `join()`: merges the states of two edges |
| 484-584 | `checkNullDereferenceStrict()`: selects the roots, computes the fixed point, and reports |

## Modified files

In source files, modified lines are enclosed by `// for lint (begin)` and
`// for lint (end)` (`#` instead of `//` in the Makefile). The line ranges below
include these markers. Markdown files have no such markers.

| File | Lines | Purpose |
|------|-------|---------|
| `README.md` | 33 | Contents: entry for the section "Static Analysis" |
| `README.md` | 851-963 | Section "Static Analysis": why there is no static analysis by default, and what a compiler has to do for such a warning |
| `test/examples.list` | 19 | Compile the programs in `analysis/examples` in `make check` |
| `Makefile` | 74-76 | Add `analysis/*.cpp` to the compiler sources |
| `abc/abc.cpp` | 8-10 | Include `analysis/analysis.hpp` |
| `abc/abc.cpp` | 113-125 | Usage message: `-Wall`, `-Wuninitialized`, `-Wnull-dereference` and `-Wnull-dereference=strict` |
| `abc/abc.cpp` | 245-260 | Parse these options; any other `-W` option is an error |
| `ast/ast.cpp` | 5-7 | Include `analysis/analysis.hpp` |
| `ast/ast.cpp` | 344-346 | `AstFuncDef::codegen()`: run the analyses once the function is complete and unreachable blocks are removed |
| `ast/ast.cpp` | 817-822 | `AstLocalVar::codegen()`: record name and location of a local variable (declaration of a single variable) |
| `ast/ast.cpp` | 832-837 | `AstLocalVar::codegen()`: record name and location of a local variable (declaration of a variable list) |
| `expr/identifier.cpp` | 4-6 | Include `analysis/analysis.hpp` |
| `expr/identifier.cpp` | 76-80 | `Identifier::loadValueFrom()`: record the source location of a read |
| `expr/member.cpp` | 3-5 | Include `<sstream>` for the source text of a read |
| `expr/member.cpp` | 7-9 | Include `analysis/analysis.hpp` |
| `expr/member.cpp` | 151-159 | `Member::loadValueFrom()`: record location and source text of a read, e.g. `list->next` |

## Limitations

- `-Wuninitialized` only tracks variables whose address never escapes
  (`llvm::isAllocaPromotable`). Struct members, array elements and variables
  with `&x` are not checked, so uninitialized reads of these are not reported.
  There are no false warnings because of them either.
- A declaration does not reset the state of a variable. A variable declared in
  a loop body and read before assignment in every iteration gets "may be used"
  instead of "is used".
- `-Wnull-dereference` only tracks local pointer variables, not parameters, not
  struct members such as `list->next`, and not variables with `&p`.
- After `q = p`, `q` counts as valid, as a later test of `p` would not apply to
  it.
- A test kept in a variable is not known: `ok = p != nullptr; if (ok) *p = 1;`
  gets a false warning.
- Functions are recognized by name, so an own allocation function that can
  return null is not known.
- `-Wnull-dereference=strict` handles the test kept in a variable and members,
  but not a flag set on a path: `ok = false; if (p) ok = true; if (ok) *p = 1;`
  gets a false warning, as this needs to follow each path separately. It warns
  about members the program guarantees to be valid in another way, and does not
  track global variables and array elements.
- A test after a dereference is only found by `-Wnull-dereference=strict`, and
  only for a pointer nothing was known about. After `q = p; *q;` a test of `p`
  gets no warning, as the dereference belongs to `q`.
