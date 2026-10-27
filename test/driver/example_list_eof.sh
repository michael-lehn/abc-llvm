# TEST: T0277
# TIMEOUT: 10
# abc-example/misc/list-example.abc stops reading at the end of input (empty
# input, numbers only) and at the first input that is not a number.

"$ABC" $ABC_FLAGS "$TOP/abc-example/misc/list-example.abc" -o list || exit 1
printf '' | ./list > empty.out || exit 1
printf '1 2 3' | ./list > numbers.out || exit 1
printf '4 5 x 6' | ./list > stop.out || exit 1
cat empty.out numbers.out stop.out
! grep -q 'val =' empty.out &&
    [ "`grep -c 'val =' numbers.out`" -eq 3 ] &&
    grep -q 'val = 5' stop.out && ! grep -q 'val = 6' stop.out
