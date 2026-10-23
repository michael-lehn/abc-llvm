# TEST: T0240
# XFAIL: #0065
# -mmcu=cortex-m3 selects the LLVM processor cortex-m3 (LLVM reports an
# unknown processor name and ignores it).

cat > f.abc <<'EOF'
fn f(x: int): int
{
    return x + 1;
}
EOF
"$ABC" $ABC_FLAGS -mmcu=cortex-m3 -c f.abc 2> err.txt || { cat err.txt; exit 1; }
cat err.txt
! grep -q 'not a recognized processor' err.txt
