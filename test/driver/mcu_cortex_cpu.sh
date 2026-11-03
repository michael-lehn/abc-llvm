# TEST: T0240
# -mmcu=cortex-m3 selects the LLVM processor cortex-m3 (LLVM reports an
# unknown processor name and ignores it).

if [ -x "$LLVM_BINDIR/llvm-config" ] &&
    ! "$LLVM_BINDIR/llvm-config" --targets-built | grep -qw ARM; then
    echo "LLVM was built without target ARM"
    exit 77
fi

cat > f.abc <<'EOF'
fn f(x: int): int
{
    return x + 1;
}
EOF
"$ABC" $ABC_FLAGS -mmcu=cortex-m3 -c f.abc 2> err.txt || { cat err.txt; exit 1; }
cat err.txt
! grep -q 'not a recognized processor' err.txt
