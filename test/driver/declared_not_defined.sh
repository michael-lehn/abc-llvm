# TEST: T0318
# A function declared without 'extern' has to be defined in the same file if it
# is used. Nothing happens if it is not used.

compile()
{
    "$ABC" $ABC_FLAGS -c "$1.abc" -o "$1.o" > "$1.txt" 2>&1
}

cat > unused.abc <<'EOF'
fn f();
fn g(x: int): int;
fn main(): int { return 0; }
EOF
compile unused || { cat unused.txt; exit 1; }
if [ -s unused.txt ]; then cat unused.txt; exit 1; fi

cat > later.abc <<'EOF'
fn f(): int;
fn g(): int { return f(); }
fn f(): int { return 0; }
fn main(): int { return g(); }
EOF
compile later || { cat later.txt; exit 1; }

cat > ext.abc <<'EOF'
extern fn f(): int;
fn main(): int { return f(); }
EOF
compile ext || { cat ext.txt; exit 1; }

cat > called.abc <<'EOF'
fn f();
fn main() { f(); }
EOF
cat > address.abc <<'EOF'
fn f();
global p: -> fn() = &f;
fn main() { }
EOF
for name in called address; do
    compile $name
    code=$?
    if [ $code -ne 1 ] || ! grep -q "function 'f' is used but not defined" $name.txt ||
	grep -q 'internal compiler error' $name.txt; then
	echo "$name: exit status $code"
	cat $name.txt
	exit 1
    fi
done
