# TEST: T0238
# XFAIL: #0063
# With -MD each input file gets its own dependency file with its own target
# and dependencies.

cat > dep1.abc <<'EOF'
@ <stdio.hdr>

fn a(): int
{
    return 0;
}
EOF
cat > dep2.abc <<'EOF'
fn b(): int
{
    return 0;
}
EOF
"$ABC" $ABC_FLAGS -c -MD dep1.abc dep2.abc || exit 1
cat dep1.d dep2.d
grep -q '^dep1\.o:.*dep1\.abc' dep1.d && grep -q 'stdio\.hdr' dep1.d &&
    grep -q '^dep2\.o:.*dep2\.abc' dep2.d && ! grep -q 'dep2\.abc' dep1.d
