# TEST: T0333
# --print-ast shows what the compiler sees of an initializer: the elements that
# are initialized, in the order of their evaluation, with a designator only where
# it is needed.

cat > f.abc <<'EOF'
struct P { x: int; y: int; };
struct U { union { a: int; b: int; }; c: int; };

fn main(): int
{
    local a0: array[3] of int = {1, 2, 3};
    local a1: array[3] of int = {[2] = 1, [0] = 2};
    local a2: array[5] of int = {1, 2, [4] = 5, [0] = 9};
    local a3: array[3] of int = {1, 2, [0] = 3};
    local a4: array[3] of int = {[1] = 7, 8};
    local p0: P = {.y = 1, .x = 2};
    local p1: P = {.y = 1};
    local p2: P = {1, 2, .x = 3};
    local u0: U = {.a = 1};
    local u1: U = {.b = 1};
    local u2: U = {.b = 1, .a = 2};
    local u3: U = {.a = 1, .b = 2};
    local u4: U = {.b = 1, .c = 2};
    local u5: U = {.c = 2};
    return 0;
}
EOF
"$ABC" $ABC_FLAGS --print-ast -c f.abc > out.txt 2>&1 || { cat out.txt; exit 1; }
check() {
    grep -qF "$1" out.txt || { echo "missing: $1"; cat out.txt; exit 1; }
}
check 'a0:array [3] of int = {1, 2, 3};'
check 'a1:array [3] of int = {2, [2] = 1};'
check 'a2:array [5] of int = {9, 2, [4] = 5};'
check 'a3:array [3] of int = {3, 2};'
check 'a4:array [3] of int = {[1] = 7, 8};'
check 'p0:P = {2, 1};'
check 'p1:P = {.y = 1};'
check 'p2:P = {3, 2};'
check 'u0:U = {1};'
check 'u1:U = {.b = 1};'
check 'u2:U = {2};'
check 'u3:U = {.b = 2};'
check 'u4:U = {.b = 1, 2};'
check 'u5:U = {.c = 2};'
