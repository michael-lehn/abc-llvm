# TEST: T0330
# XFAIL: #0096
# Aggregates with many elements can be handled by value, also for x86-64.

if [ -x "$LLVM_BINDIR/llvm-config" ] &&
    ! "$LLVM_BINDIR/llvm-config" --targets-built | grep -qw X86; then
    echo "LLVM was built without target X86"
    exit 77
fi

cat > big.abc <<'EOF'
fn main()
{
    local z: array[] of int = {[70000] = 1};
}
EOF
"$ABC" $ABC_FLAGS -target x86_64-unknown-linux-gnu -c big.abc -o big.o
