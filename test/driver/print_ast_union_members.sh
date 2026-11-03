# TEST: T0329
# --print-ast of unions, also where the last member list has several names.

cat > u.abc <<'EOF'
struct S
{
    union {
	a: int;
	b, c: float;
    };
};

struct T
{
    k: int;
    union {
	d, e: int;
	f: double;
    };
    l: int;
};
EOF
"$ABC" $ABC_FLAGS --print-ast -c u.abc -o u.o > out.txt 2>&1 || { cat out.txt; exit 1; }
for text in 'b, c: float;' 'd, e: int;' 'f: double;' 'l: int;'; do
    grep -q "$text" out.txt || { echo "missing: $text"; cat out.txt; exit 1; }
done
[ `grep -c 'union {' out.txt` = 2 ] || { cat out.txt; exit 1; }
