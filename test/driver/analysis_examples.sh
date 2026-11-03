# TEST: T0289
# The programs in analysis/examples say in a comment which options generate a
# warning, and mark the lines with the expected message.  The comment has to be
# correct: each listed option gives exactly these warnings, every other option
# none.  The comment 'Warning options: none' means no warning with any option.

options="-Wuninitialized -Wnull-dereference -Wnull-dereference=strict -Wall"
esc=`printf '\033'`
status=0

for f in "$TOP"/analysis/examples/*.abc; do
    name=`basename "$f"`
    listed=`sed -n 's|^// Warning options: *||p' "$f" | tr -d ','`
    if [ -z "$listed" ]; then
	echo "$name: no comment 'Warning options:'"
	status=1
	continue
    fi
    sed -n '/WARNING: /{=;s/^.*WARNING: //;p;}' "$f" | paste -d' ' - - | sort \
	> expected.txt
    for option in "" $options; do
	if ! "$ABC" $ABC_FLAGS $option -c "$f" -o example.o 2> err.txt; then
	    echo "$name: not compiled with '$option'"
	    cat err.txt
	    status=1
	    continue
	fi
	sed "s/$esc\[[0-9;]*m//g" err.txt |
	    sed -n "s/^.*$name:\([0-9]*\)\.[^:]*: *warning: *\(.*\)$/\1 \2/p" |
	    sort > got.txt
	case " $listed " in
	*" $option "*)
	    [ -n "$option" ] && want=expected.txt || want=/dev/null
	    ;;
	*)
	    want=/dev/null
	    ;;
	esac
	if ! diff "$want" got.txt; then
	    echo "$name: wrong warnings with '$option' (expected: $want)"
	    status=1
	fi
    done
done
exit $status
