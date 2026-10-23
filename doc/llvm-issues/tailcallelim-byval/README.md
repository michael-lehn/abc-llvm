# LLVM miscompile: tailcallelim, byval and the inliner

Reported upstream as
[llvm/llvm-project#235074](https://github.com/llvm/llvm-project/issues/235074)
(submitted 2026-10-23). `issue.md` is the submitted report; `repro.ll` is the
standalone reproducer from the report. It does not depend on ABC.

The bug was found with test T0196 (`test/run/param_aggregate_modify.abc`) and
is the reason why ABC marks calls with `byval` arguments `notail`
(`gen/abi.cpp`, `lowerFunctionCall`; bug #0036 in
`test/bugs_and_open_design_questions.md`, regression test T0209).

## Reproducing

With the tools of the LLVM installation to test (e.g. `$(brew --prefix
llvm@23)/bin`):

```sh
opt -passes=verify -disable-output repro.ll   # valid IR
lli repro.ll; echo $?                         # 5: correct (no optimization)
opt -passes='function(tailcallelim),cgscc(inline)' repro.ll -o r.bc
lli r.bc; echo $?                             # 0: miscompiled
clang -O2 repro.ll -o r && ./r; echo $?       # 0: miscompiled
```

Expected is 5 in all cases. Results (2026-10-23): LLVM 17.0.6 to 23.1.4
(Homebrew, arm64) and Clang trunk b3f0c0ca9469 (Compiler Explorer, x86-64)
give 0 for the optimized variants.
