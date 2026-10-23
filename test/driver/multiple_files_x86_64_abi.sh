# TEST: T0234
# XFAIL: #0059
# Compiling several files for x86-64 in one invocation.  The x86-64 ABI code
# keeps a pointer to an interned type across input files (it is freed when
# the type system is reset for the next file): a later file is miscompiled
# (a 4-byte struct passed in memory instead of a register) or the compiler
# crashes.  Expected: each object file is the same as when the file is
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
cp rgba.abc alone/rgba.abc
(cd alone && "$ABC" $ABC_FLAGS -target x86_64-unknown-linux-gnu -c rgba.abc) \
    || exit 1
for n in 2 3 4 5; do
    cp rgba.abc s$n.abc
done
"$ABC" $ABC_FLAGS -target x86_64-unknown-linux-gnu \
    -c first.abc s2.abc s3.abc s4.abc s5.abc || exit 1
for n in 2 3 4 5; do
    cmp s$n.o alone/rgba.o || exit 1
done
