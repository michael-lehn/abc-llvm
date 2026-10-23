# TEST: T0236
# XFAIL: #0061
# With -c an input file that is not an ABC source file is not used (warning)
# and not compiled as ABC; the ABC file is compiled.

cat > ok.abc <<'EOF'
fn main(): int
{
    return 0;
}
EOF
echo 'int x;' > other.c
"$ABC" $ABC_FLAGS -c ok.abc other.c 2> err.txt
status=$?
cat err.txt
test $status -eq 0 && test -s ok.o && ! test -e other.o
