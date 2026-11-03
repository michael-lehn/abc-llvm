# TEST: T0234
# Compiling several files for x86-64 in one invocation.  The x86-64 ABI code
# kept a pointer to an interned type across input files (it is freed when the
# type system is reset for the next file): a later file was miscompiled (a
# 4-byte struct passed in memory instead of a register) or the compiler crashed
# (issue #0059).  Expected: each object file is the same as when the file is
# compiled alone.  Several files make the failure independent of the heap
# layout of the compiler.

cat > first.abc <<'EOF'
fn one(x: int): int
{
    return x;
}
EOF
cat > rgba.abc <<'EOF'
struct Rgba
{
    r, g, b, a: u8;
};

fn red(c: Rgba): int
{
    return c.r;
}
EOF
mkdir alone
for n in 2 3 4 5; do
    cp rgba.abc s$n.abc
    cp rgba.abc alone/s$n.abc
    # an object file contains the name of its module
    (cd alone && "$ABC" $ABC_FLAGS -target x86_64-unknown-linux-gnu -c s$n.abc) \
	|| exit 1
done
"$ABC" $ABC_FLAGS -target x86_64-unknown-linux-gnu \
    -c first.abc s2.abc s3.abc s4.abc s5.abc || exit 1
for n in 2 3 4 5; do
    cmp s$n.o alone/s$n.o || exit 1
done
