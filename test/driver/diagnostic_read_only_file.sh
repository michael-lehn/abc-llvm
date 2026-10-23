# TEST: T0239
# XFAIL: #0064
# Diagnostics show the source line also for files without write permission.

printf 'fn main(): int\n{\n    return 0 +;\n}\n' > ro.abc
chmod a-w ro.abc
"$ABC" $ABC_FLAGS -c ro.abc 2> err.txt
cat err.txt
grep -q 'return 0 +;' err.txt
