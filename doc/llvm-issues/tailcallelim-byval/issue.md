**Title:** [TailCallElim] Writes to byval argument leave function memory(argmem: read) in place; inliner then elides the byval copy

---

When `tailcallelim` eliminates a self-recursive tail call that passes an
argument `byval`, it copies the new argument value into the function's own
`byval` argument. It removes `readonly` from that parameter (#64289), but it
does not update the function's memory effects: a previously correct
`memory(argmem: read)` stays on a function that now writes through its pointer
argument. The inliner trusts `CalledFunc->onlyReadsMemory()` when it decides
whether the `byval` copy can be elided, so after inlining the eliminated
recursion writes directly into the caller's object.

### Reproducer

`@f` only reads `%p`, so `memory(argmem: read)` is correct for the input. Since
`%s` is passed `byval`, the call cannot modify it; `main` must return 5.

```llvm
%T = type { i64, i32 }

define internal i64 @f(ptr byval(%T) align 8 %p) memory(argmem: read) {
entry:
  %local = alloca %T, align 8
  %v = load %T, ptr %p, align 8
  store %T %v, ptr %local, align 8
  %n.addr = getelementptr inbounds %T, ptr %local, i32 0, i32 1
  %n = load i32, ptr %n.addr, align 8
  %done = icmp eq i32 %n, 0
  br i1 %done, label %exit, label %recurse

exit:
  ret i64 0

recurse:
  store i32 0, ptr %n.addr, align 8
  %r = call i64 @f(ptr byval(%T) align 8 %local)
  ret i64 %r
}

define i32 @main() {
entry:
  %s = alloca %T, align 8
  store %T { i64 1, i32 5 }, ptr %s, align 8
  %r = call i64 @f(ptr byval(%T) align 8 %s)
  %n.addr = getelementptr inbounds %T, ptr %s, i32 0, i32 1
  %n = load volatile i32, ptr %n.addr, align 8
  ret i32 %n    ; 5 = correct, 0 = main's object was modified
}
```

```
$ lli repro.ll; echo $?
5
$ opt -passes='function(tailcallelim),cgscc(inline)' repro.ll -o r.bc && lli r.bc; echo $?
0
$ clang -O2 repro.ll -o r && ./r; echo $?
0
```

**Expected:** 5 (`@f` operates on a private copy of `%s`).
**Actual:** 0 (the inlined loop overwrites `%s`).

The IR passes the verifier before and after the transformation
(`-verify-each`); `clang -O0`/`-O1` give 5.

Without the explicit attribute, `function-attrs` infers it, so
`-passes='cgscc(function-attrs),function(tailcallelim),cgscc(inline)'` and
`clang -O2` fail the same way.

### What happens

After `tailcallelim`:

```llvm
define internal i64 @f(ptr byval(%T) align 8 %p) #0 {
  ...
recurse:
  store i32 0, ptr %n.addr, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %local1, ptr align 8 %local, i64 12, i1 false)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %p, ptr align 8 %local1, i64 12, i1 false)
  br label %tailrecurse
}
attributes #0 = { memory(argmem: read) }
```

The `memcpy` into `%p` is an `argmem` write (LangRef: `argmem` covers
"accesses that are based on pointer arguments to the function"; and for
`byval`: "`readonly` functions should not write to `byval` parameters"). Running
`function-attrs` from scratch on this function infers
`memory(argmem: readwrite)`; running it again after `tailcallelim` does not
repair the existing attribute.

`inline` then treats `@f` as read-only, maps `%p` to `%s` without a copy, and
the inlined `memcpy(ptr %s, ...)` modifies `main`'s object:

```llvm
recurse.i:
  store i32 0, ptr %n.addr.i, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %local1.i, ptr align 8 %local.i, i64 12, i1 false)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %s, ptr align 8 %local1.i, i64 12, i1 false)
  br label %tailrecurse.i
```

### Source locations (at b3f0c0ca9469)

- `llvm/lib/Transforms/Scalar/TailRecursionElimination.cpp:908-917`
  (`eliminateCall`): calls `copyLocalTempOfByValueOperandIntoArguments` and
  `F.removeParamAttr(I, Attribute::ReadOnly)`; the comment states that "from the
  perspective of a caller, the byval parameter is always treated as
  'readonly'". The function-level memory effects are not changed anywhere in
  the pass.
- `llvm/lib/Transforms/Utils/InlineFunction.cpp:1862-1865`
  (`HandleByValArgument`): "If the called function is readonly, then it could
  not mutate the caller's copy of the byval'd memory. In this case, it is safe
  to elide the copy and temporary." — `if (CalledFunc->onlyReadsMemory())`.

### Tested versions

- clang/opt/lli 17.0.6, 18.1.8, 19.1.7, 20.1.8, 21.1.8, 22.1.8, 23.1.4
  (arm64-apple-darwin, Homebrew builds): all affected (`lli` 5, `opt` pipeline
  above 0, `clang -O2` 0).
- Compiler Explorer clang/opt trunk, `clang version 24.0.0git`
  (b3f0c0ca9469, x86_64-unknown-linux-gnu), with and without assertions:
  `clang -O0` 5, `clang -O2` 0; `opt -passes='function(tailcallelim),cgscc(inline)'`
  produces the `memcpy` into `%s` shown above. No assertion fires.

### Related issues

- #64289: `tailcallelim` wrote to a `readonly byval` parameter; fixed by
  removing the parameter-level `readonly`. This report is about the
  function-level `memory(...)` attribute, which that fix does not touch.
- #232387: the inliner's `byval` copy elision for a callee that really is
  read-only (the callee can still observe pointer identity). Here the
  attribute itself is wrong after `tailcallelim`. Of the conditions proposed
  there, `captures(none)` would not prevent this case (`%p` is not captured);
  restricting the elision to parameters whose uses are plain loads would.
- #234334 / #234699: `tailcallelim` copies the new byval value back before
  instructions that follow the self-call. Not involved here (nothing follows
  the call); #234699 only moves the copy-back and does not change attributes.

### Possible fix (suggestion, not a tested patch)

When `eliminateCall` writes to a `byval` argument, also weaken the function's
memory effects, e.g. add `ModRef` for `argmem` (or drop the `memory`
attribute), next to the existing `removeParamAttr(I, Attribute::ReadOnly)`.
Alternatively, `HandleByValArgument` could base the elision on the specific
argument (e.g. the parameter being `readonly`) instead of
`CalledFunc->onlyReadsMemory()`.
