# Warnings in `not-abc`

Programs in this directory that generate warnings with the options for static
analysis, which are off by default (see
[Static Analysis](../README.md#static-analysis)). All other programs in this
directory generate none.

To see a warning:

```bash
abc -Wall -c FILE
```

14 warnings in 4 files:

| File | Line | Warning | Options |
|------|------|---------|---------|
| `expr.abc` | 9 | pointer 'n' may be null when dereferenced | `-Wnull-dereference`, `-Wnull-dereference=strict`, `-Wall` |
| `expr.abc` | 17 | pointer 'n' may be null when dereferenced | `-Wnull-dereference`, `-Wnull-dereference=strict`, `-Wall` |
| `expr.abc` | 26 | pointer 'n' may be null when dereferenced | `-Wnull-dereference`, `-Wnull-dereference=strict`, `-Wall` |
| `expr.abc` | 34 | pointer 'n' may be null when dereferenced | `-Wnull-dereference`, `-Wnull-dereference=strict`, `-Wall` |
| `expr.abc` | 42 | pointer 'n' may be null when dereferenced | `-Wnull-dereference`, `-Wnull-dereference=strict`, `-Wall` |
| `expr.abc` | 52 | pointer 'n' may be null when dereferenced | `-Wnull-dereference`, `-Wnull-dereference=strict`, `-Wall` |
| `expr.abc` | 258 | pointer 'expr->binary.left' may be null when dereferenced | `-Wnull-dereference=strict`, `-Wall` |
| `gen_llvm.abc` | 176 | pointer 'n' may be null when dereferenced | `-Wnull-dereference`, `-Wnull-dereference=strict`, `-Wall` |
| `gen_llvm.abc` | 376 | pointer 'n' may be null when dereferenced | `-Wnull-dereference`, `-Wnull-dereference=strict`, `-Wall` |
| `gen_llvm.abc` | 386 | pointer 'n' may be null when dereferenced | `-Wnull-dereference`, `-Wnull-dereference=strict`, `-Wall` |
| `gen_llvm.abc` | 446 | pointer 'n' may be null when dereferenced | `-Wnull-dereference`, `-Wnull-dereference=strict`, `-Wall` |
| `gen_llvm.abc` | 450 | pointer 'n->cStr' may be null when dereferenced | `-Wnull-dereference=strict`, `-Wall` |
| `gen_simple.abc` | 444 | pointer 'n' may be null when dereferenced | `-Wnull-dereference`, `-Wnull-dereference=strict`, `-Wall` |
| `ustr.abc` | 29 | pointer 'n' may be null when dereferenced | `-Wnull-dereference`, `-Wnull-dereference=strict`, `-Wall` |
