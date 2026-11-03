# Warnings in `abc-example/misc`

Programs in this directory that generate warnings with the options for static
analysis, which are off by default (see
[Static Analysis](../../README.md#static-analysis)). All other programs in this
directory generate none.

To see a warning:

```bash
abc -Wall -c FILE
```

7 warnings in 4 files:

| File | Line | Warning | Options |
|------|------|---------|---------|
| `const.abc` | 45 | variable 'arr1' is used uninitialized | `-Wuninitialized`, `-Wall` |
| `list-example.abc` | 22 | pointer 'n' may be null when dereferenced | `-Wnull-dereference`, `-Wnull-dereference=strict`, `-Wall` |
| `list.abc` | 14 | pointer 'n' may be null when dereferenced | `-Wnull-dereference`, `-Wnull-dereference=strict`, `-Wall` |
| `typealias.abc` | 9 | variable 'a' is used uninitialized | `-Wuninitialized`, `-Wall` |
| `typealias.abc` | 9 | variable 'b' is used uninitialized | `-Wuninitialized`, `-Wall` |
| `typealias.abc` | 37 | variable 'a' is used uninitialized | `-Wuninitialized`, `-Wall` |
| `typealias.abc` | 37 | variable 'b' is used uninitialized | `-Wuninitialized`, `-Wall` |
