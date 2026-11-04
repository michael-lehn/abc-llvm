# TEST: T0219
# Printing an initializer that overrides a previous initialization (the compiler
# warns about it) does not crash, neither with --print-ast nor with -E, nor for
# the message of an error or of an assertion. What is printed is what the compiler
# sees: the initializer that is used.

cat > f.abc <<'EOF'
fn main(): int
{
    local a: array[2] of int = {[0] = 1, [0] = 2};
    return a[0] - 2;
}
EOF
"$ABC" $ABC_FLAGS --print-ast -c f.abc > ast.txt 2>&1
status=$?
cat ast.txt
test $status -eq 0 && grep -q '= {2};' ast.txt && test -s f.o || exit 1

"$ABC" $ABC_FLAGS -E f.abc > e.txt 2>&1 || { cat e.txt; exit 1; }
grep -q '= {2};' e.txt || { cat e.txt; exit 1; }

cat > err.abc <<'EOF'
struct S { x: int; };
fn main(): int
{
    local s: S;
    return s + (S){.x = 1, .x = 2};
}
EOF
"$ABC" $ABC_FLAGS -c err.abc > err.txt 2>&1
status=$?
cat err.txt
test $status -eq 1 && grep -q '(S){2}' err.txt || exit 1

cat > as.abc <<'EOF'
struct S { x: int; };
fn main(): int
{
    assert(((S){.x = 1, .x = 2}).x == 2);
    return 0;
}
EOF
"$ABC" $ABC_FLAGS -S -emit-llvm as.abc -o as.ll > as.txt 2>&1 || { cat as.txt; exit 1; }
grep -q '(S){2}' as.ll || { cat as.ll; exit 1; }
