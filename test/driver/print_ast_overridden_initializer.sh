# TEST: T0219
# XFAIL: #0045
# Printing the AST (--print-ast) of an initializer that overrides a previous
# initialization (the compiler warns about it): the initializer is printed as
# written and the compiler does not crash.

cat > f.abc <<'EOF'
fn main(): int
{
    local a: array[2] of int = {[0] = 1, [0] = 2};
    return a[0] - 2;
}
EOF
"$ABC" $ABC_FLAGS --print-ast -c f.abc > ast.txt
status=$?
cat ast.txt
test $status -eq 0 && grep -q '{\[0\] = 1, \[0\] = 2}' ast.txt && test -s f.o
