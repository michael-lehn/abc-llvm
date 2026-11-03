# TEST: T0292
# long has the size of the C type of the target, not of the machine the compiler
# runs on: 32 bits on a 32-bit target, 64 bits on a 64-bit target. long_long has
# 64 bits on both.

cat > f.abc <<'EOF'
global l: long = 1;
global ul: unsigned_long = 1;
global ll: long_long = 1;
global ull: unsigned_long_long = 1;
EOF

check()
{
    triple=$1
    bits=$2
    "$ABC" $ABC_FLAGS -target "$triple" --emit-llvm f.abc -o f.ll 2> err.txt ||
	{ cat err.txt; exit 1; }
    for line in "@l = internal global i$bits 1" "@ul = internal global i$bits 1" \
	"@ll = internal global i64 1" "@ull = internal global i64 1"; do
	if ! grep -q "^$line" f.ll; then
	    echo "$triple: missing '$line'"
	    cat f.ll
	    exit 1
	fi
    done
}

targets=`"$LLVM_BINDIR/llvm-config" --targets-built`
case " $targets " in
*" X86 "*)
    check x86_64-unknown-linux-gnu 64
    check i386-unknown-linux-gnu 32
    ;;
*" AArch64 "*)
    check aarch64-unknown-linux-gnu 64
    ;;
*)
    echo "LLVM was built without target X86 and AArch64"
    exit 77
    ;;
esac
