# TEST: T0235
# XFAIL: #0060
# Two input files with the same name in different directories are compiled
# into different temporary object files and linked together.

mkdir d1 d2
cat > d1/same.abc <<'EOF'
extern fn helper(): int;

fn main(): int
{
    return helper() == 2 ? 0 : 1;
}
EOF
cat > d2/same.abc <<'EOF'
extern fn helper(): int;

fn helper(): int
{
    return 2;
}
EOF
"$ABC" $ABC_FLAGS -L"$ABC_BUILD" d1/same.abc d2/same.abc -o prog || exit 1
./prog
