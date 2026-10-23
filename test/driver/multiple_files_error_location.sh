# TEST: T0237
# XFAIL: #0062
# A syntax error at the end of the second input file is reported for that
# file, not for the first one.

cat > first.abc <<'EOF'
fn f(): int
{
    return 1;
}
EOF
printf 'fn main(): int\n{\n    return 0;\n' > second.abc
"$ABC" $ABC_FLAGS -c first.abc second.abc 2> err.txt
status=$?
cat err.txt
test $status -ne 0 && grep -q 'second\.abc:' err.txt && ! grep -q 'first\.abc:' err.txt
