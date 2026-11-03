# TEST: T0300
# The definition of an extern array without dimension keeps external linkage, so
# another file can use it, and it has the complete type.

cat > def.abc <<'EOF'
extern a: array[] of int;
global a: array[] of int = {1, 2, 3};
EOF
"$ABC" $ABC_FLAGS --emit-llvm def.abc -o def.ll 2> err.txt || { cat err.txt; exit 1; }
if ! grep -q '^@a = global \[3 x i32\] \[i32 1, i32 2, i32 3\]' def.ll; then
    cat def.ll
    exit 1
fi
