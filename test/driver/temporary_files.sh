# TEST: T0321
# Temporary object files: compilers running at the same time do not use each
# other's files, and nothing is left behind, also after an error.

mkdir tmp a b
TMPDIR=$PWD/tmp
export TMPDIR

empty()
{
    if [ -n "`ls tmp`" ]; then
	echo "left in the temporary directory after $1:"
	ls -R tmp
	exit 1
    fi
}

cat > a/p.abc <<'EOF'
fn main(): int
{
    return 11;
}
EOF
cat > b/p.abc <<'EOF'
fn main(): int
{
    return 22;
}
EOF

# at the same time, with the same name of the input file
round=0
while [ $round -lt 12 ]; do
    "$ABC" $ABC_FLAGS -L"$ABC_BUILD" a/p.abc -o a/prog$round &
    a=$!
    "$ABC" $ABC_FLAGS -L"$ABC_BUILD" b/p.abc -o b/prog$round &
    b=$!
    wait $a || { echo "a: compiler failed"; exit 1; }
    wait $b || { echo "b: compiler failed"; exit 1; }
    a/prog$round
    code=$?
    [ $code -eq 11 ] || { echo "a/prog$round: exit status $code"; exit 1; }
    b/prog$round
    code=$?
    [ $code -eq 22 ] || { echo "b/prog$round: exit status $code"; exit 1; }
    round=`expr $round + 1`
done
empty "successful builds"

# failing in the linker
cat > undefined.abc <<'EOF'
extern fn notDefinedAnywhere();
fn main()
{
    notDefinedAnywhere();
}
EOF
if "$ABC" $ABC_FLAGS -L"$ABC_BUILD" undefined.abc -o undefined 2> /dev/null; then
    echo "linked with an undefined function"
    exit 1
fi
empty "a linker error"

# the first file is compiled, the second has an error
cat > bad.abc <<'EOF'
fn f(: int;
EOF
if "$ABC" $ABC_FLAGS -L"$ABC_BUILD" a/p.abc bad.abc -o bad 2> /dev/null; then
    echo "compiled a program with an error"
    exit 1
fi
empty "a compile error"

# an object file given by the user stays, and -c does not use the directory
"$ABC" $ABC_FLAGS -c a/p.abc -o user.o || exit 1
empty "-c"
"$ABC" $ABC_FLAGS -L"$ABC_BUILD" user.o -o fromObject || exit 1
[ -f user.o ] || { echo "object file of the user removed"; exit 1; }
empty "linking an object file"
fromObject=`pwd`/fromObject
"$fromObject"
[ $? -eq 11 ] || { echo "fromObject: wrong exit status"; exit 1; }
empty "running the program"

# directories left by another run or another user are not touched
mkdir tmp/abc-0 tmp/abc-1
echo keep > tmp/abc-0/p.o
"$ABC" $ABC_FLAGS -L"$ABC_BUILD" a/p.abc -o leftover || exit 1
./leftover
[ $? -eq 11 ] || { echo "leftover: wrong exit status"; exit 1; }
[ "`cat tmp/abc-0/p.o`" = keep ] || { echo "file of another run changed"; exit 1; }
[ "`ls tmp`" = "abc-0
abc-1" ] || { echo "wrong contents of the temporary directory:"; ls tmp; exit 1; }

# no usable temporary directory: an ordinary error
expect_error()
{
    "$ABC" $ABC_FLAGS -L"$ABC_BUILD" a/p.abc -o never > err.txt 2>&1
    code=$?
    if [ $code -ne 1 ] || ! grep -q 'can not create a temporary directory' err.txt; then
	echo "$1: exit status $code"
	cat err.txt
	exit 1
    fi
}
TMPDIR=$PWD/missing
export TMPDIR
expect_error "missing directory"

mkdir readonly
chmod 555 readonly
if [ ! -w readonly ]; then
    TMPDIR=$PWD/readonly
    export TMPDIR
    expect_error "read-only directory"
fi
chmod 755 readonly
