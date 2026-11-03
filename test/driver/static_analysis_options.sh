# TEST: T0281
# No static analysis by default. It is enabled with -Wall or -W<name>, and it
# does not change the generated code.

cat > uninit.abc <<'EOF'
fn main(): int
{
    local x: int;
    return x;
}
EOF

message='uninit.abc:4.12-4.13: .*variable .*x.* is used uninitialized'

"$ABC" $ABC_FLAGS -c uninit.abc -o default.o 2> default.txt || exit 1
if test -s default.txt; then
    echo "diagnostics without -W option:"
    cat default.txt
    exit 1
fi

for option in -Wall -Wuninitialized; do
    "$ABC" $ABC_FLAGS $option -c uninit.abc -o enabled.o 2> enabled.txt || exit 1
    if ! grep -q "$message" enabled.txt; then
	echo "no warning with $option:"
	cat enabled.txt
	exit 1
    fi
done

"$ABC" $ABC_FLAGS --emit-llvm uninit.abc -o default.ll 2> /dev/null || exit 1
"$ABC" $ABC_FLAGS -Wall --emit-llvm uninit.abc -o enabled.ll 2> /dev/null || exit 1
if ! cmp default.ll enabled.ll; then
    echo "-Wall changes the generated code"
    exit 1
fi

if "$ABC" $ABC_FLAGS -Wnot-an-analysis -c uninit.abc -o unknown.o 2> /dev/null; then
    echo "unknown -W option accepted"
    exit 1
fi

"$ABC" --help 2> help.txt
grep -q -- '-Wall' help.txt && grep -q -- '-Wuninitialized' help.txt &&
    grep -q -- '-Wnull-dereference' help.txt
