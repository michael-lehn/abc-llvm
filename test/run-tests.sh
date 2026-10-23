#!/bin/sh
#
# Test driver for the ABC compiler.  Usually invoked through 'make check'.
#
# Every test has a stable ID (// TEST: Tnnnn) that does not depend on its file
# name.  Known compiler bugs and open language design questions are listed in
# test/bugs_and_open_design_questions.md; tests refer to them by issue ID
# (#nnnn).  test/validate-tests.pl checks the uniqueness of the IDs and the
# consistency of all references before the tests are run.
#
# Test kinds:
#
#   UNIT_TESTS       C++ unit test executables (test/unit/*.cpp).  A unit test
#                    passes if it exits with status 0.  Checks of known bugs
#                    (CHECK_KNOWN_BUG) have their own test IDs.
#
#   test/run/*.abc   Programs that must compile, produce LLVM IR accepted by
#                    the LLVM verifier, and run successfully (at every
#                    optimization level in ABC_TEST_OPT).  If test/run/NAME.out
#                    exists, the standard output of the program must match it
#                    exactly.  If test/run/NAME.c exists, it is compiled with
#                    'cc' and linked into the program (C interoperability).
#
#   test/fail/*.abc  Programs that must be rejected by the compiler with a
#                    diagnostic, without crashing or hanging.
#
#   test/design/*.abc Programs concerning an open language design question.
#                    The observed behavior is ACCEPTED (compiles, IR is
#                    valid, and the program runs successfully) or REJECTED
#                    (the compiler reports an error).  It must match the
#                    recorded behavior; a crash, a hang, or a program that
#                    compiles but fails is an error.
#
#   test/ir/*.abc    Programs compiled with the options of their ARGS
#                    directive; the LLVM IR must be valid, match the IR-MATCH
#                    patterns, and FUNCTION must have the expected size
#                    attributes and optimization (see test/check-ir.pl).
#
#   test/tool/*      Scripts checking the build environment, e.g. the
#                    compilation database (compile_commands.json) and,
#                    optionally, clangd.  Exit status 0: PASS, 77: SKIP.
#
# Classification of results:
#
#   PASS    expected behavior (including correct rejection of invalid code)
#   XFAIL   fails because of a known compiler bug (OPEN issue of type BUG)
#   XPASS   test of a known bug passes: an error, update the test and issue
#   DESIGN  open design question (OPEN issue of type DESIGN), observed
#           behavior as recorded
#   FAIL    an error
#   SKIP    test not applicable on this machine
#
# Directives are written as comments at the beginning of a line of a test:
#
#   // TEST: Tnnnn       stable test ID (required)
#   // EXIT: n           expected exit status of a run test (default 0)
#   // ERROR: regex      extended regex (case-insensitive) the diagnostic of a
#                        fail test must match (required for fail tests)
#   // XFAIL: #nnnn ...  known compiler bug #nnnn; the test is expected to fail
#   // DESIGN: #nnnn ... design test for open question #nnnn (test/design)
#   // OBSERVED: ACCEPTED | REJECTED
#                        recorded behavior of a design test
#   // REQUIRES: arch    only run on machines where 'uname -m' prints arch
#   // TIMEOUT: secs     time limit for each compiler and program invocation
#   // ARGS: options     compiler options of an IR test (test/ir; %s: source
#                        file, %o: output file) or additional options of a
#                        runtime or reject test (test/run, test/fail; appended
#                        after the file); %b stands for the build directory
#   // IR-MATCH: regex   a line of the IR of an IR test must match (repeatable)
#   // FUNCTION: name    function checked by an IR test (optional, requires
#                        SIZE-ATTRIBUTES and OPTIMIZED)
#   // SIZE-ATTRIBUTES: none | optsize | optsize minsize
#                        expected size attributes of FUNCTION (IR test)
#   // OPTIMIZED: yes | no
#                        whether FUNCTION is optimized (no alloca) (IR test)
#
# Tool checks (test/tool) declare their ID with a line '# TEST: Tnnnn'.
#
# Environment (defaults work from the top-level directory after 'make'):
#
#   ABC            compiler executable              (build/abc/abc)
#   ABC_INCLUDE    directory with ABC headers       (abc-include)
#   ABC_BUILD      build directory (libabc.a, ...)  (build/)
#   LLVM_BINDIR    directory containing 'opt'       (empty: IR not verified)
#   UNIT_TESTS     unit test executables            (none)
#   ABC_TEST_OPT   optimization levels for run tests ("-O0 -O2")
#   ABC_TEST_FILTER only run tests whose path or ID contains this string
#
# Generated files and logs of each test are kept in $ABC_BUILD/check.
# The script must be run from the top-level directory of the repository.
#

LC_ALL=C
export LC_ALL

top=`pwd`
ABC=${ABC:-build/abc/abc}
ABC_INCLUDE=${ABC_INCLUDE:-abc-include}
ABC_BUILD=${ABC_BUILD:-build/}
ABC_TEST_OPT=${ABC_TEST_OPT:--O0 -O2}
default_timeout=20

case $ABC in /*) ;; *) ABC=$top/$ABC ;; esac
case $ABC_INCLUDE in /*) ;; *) ABC_INCLUDE=$top/$ABC_INCLUDE ;; esac
case $ABC_BUILD in /*) ;; *) ABC_BUILD=$top/$ABC_BUILD ;; esac
ABC_BUILD=${ABC_BUILD%/}

work=$ABC_BUILD/check
abc_flags="-I $ABC_INCLUDE -I $ABC_BUILD -I $top/test/include"

if [ ! -x "$ABC" ]; then
    echo "error: compiler '$ABC' not found (run 'make' first)" >&2
    exit 2
fi
if ! command -v perl > /dev/null 2>&1; then
    echo "error: perl is required" >&2
    exit 2
fi

opt=
if [ -n "$LLVM_BINDIR" ] && [ -x "$LLVM_BINDIR/opt" ]; then
    opt=$LLVM_BINDIR/opt
else
    echo "note: LLVM 'opt' not found, generated IR is not verified"
fi

arch=`uname -m`

n_pass=0
n_fail=0
n_xfail=0
n_xpass=0
n_design=0
n_skip=0
failed_tests=

rm -rf "$work"
mkdir -p "$work"

# run_limited SECONDS CMD ARGS...: run command with a time limit (status 142
# if the limit is exceeded)
run_limited()
{
    limit=$1
    shift
    perl -e 'alarm shift @ARGV; exec @ARGV or exit 127' "$limit" "$@"
}

# directive FILE NAME: print value of directive '// NAME: value' in FILE
directive()
{
    sed -n "s|^// $2: *||p" "$1" | head -n 1
}

# issue FILE NAME: issue ID (#nnnn) of directive NAME (XFAIL or DESIGN)
issue()
{
    directive "$1" "$2" | sed -n 's|^\(#[0-9]*\).*|\1|p'
}

# strip ANSI color sequences from compiler diagnostics
strip_color()
{
    perl -pe 's/\e\[[0-9;]*m//g'
}

# describe_status STATUS: human readable description of an exit status
describe_status()
{
    case $1 in
    142) echo "timed out" ;;
    134) echo "aborted (exit status 134)" ;;
    139) echo "segmentation fault (exit status 139)" ;;
    *) echo "exit status $1" ;;
    esac
}

# result CLASS ID NAME [DETAIL]: print one result line and count it
result()
{
    printf '%-6s %s %s%s\n' "$1" "$2" "$3" "${4:+ $4}"
    case $1 in
    PASS) n_pass=`expr $n_pass + 1` ;;
    XFAIL) n_xfail=`expr $n_xfail + 1` ;;
    DESIGN) n_design=`expr $n_design + 1` ;;
    SKIP) n_skip=`expr $n_skip + 1` ;;
    XPASS)
	n_xpass=`expr $n_xpass + 1`
	failed_tests="$failed_tests $2"
	;;
    *)
	n_fail=`expr $n_fail + 1`
	failed_tests="$failed_tests $2"
	;;
    esac
}

# show_log LOG: print the beginning of a log file
show_log()
{
    if [ -n "$1" ] && [ -s "$1" ]; then
	strip_color < "$1" | head -n 30 | sed 's/^/    | /'
    fi
}

# report ID NAME XFAIL_ISSUE FAILURE_MESSAGE LOG
#   FAILURE_MESSAGE is empty if the test passed.
report()
{
    if [ -z "$4" ]; then
	if [ -n "$3" ]; then
	    result XPASS "$1" "$2" "[$3] known bug seems to be fixed"
	else
	    result PASS "$1" "$2"
	fi
    elif [ -n "$3" ]; then
	result XFAIL "$1" "$2" "[$3]"
    else
	result FAIL "$1" "$2" "$4"
	show_log "$5"
    fi
}

selected()
{
    [ -z "$ABC_TEST_FILTER" ] && return 0
    case "$1 $2" in *"$ABC_TEST_FILTER"*) return 0 ;; esac
    return 1
}

#-- validation of test IDs and issue references --------------------------------

if ! perl "$top/test/validate-tests.pl" "$top/test" $UNIT_TESTS \
	> "$work/validate.log"
then
    result FAIL "-----" "test IDs and issue references"
    sed 's/^/    | /' "$work/validate.log"
fi

#-- unit tests -----------------------------------------------------------------

for t in $UNIT_TESTS; do
    base=`basename $t`
    name=unit/$base
    id=`directive "$top/test/unit/$base.cpp" TEST`
    selected "$name" "$id" || continue
    log=$work/unit-$base.log
    mkdir -p "$work/unit-tmp"
    case $t in /*) ;; *) t=$top/$t ;; esac
    TMPDIR=$work/unit-tmp run_limited $default_timeout "$t" > "$log" 2>&1
    status=$?
    # checks of known bugs: "XFAIL|XPASS <test> <issue> <file>:<line>: <cond>"
    n_known_fixed=0
    while read -r class kid kissue where rest; do
	case $class in
	XFAIL) result XFAIL "$kid" "$name ${where%:}" "[$kissue]" ;;
	XPASS)
	    result XPASS "$kid" "$name ${where%:}" \
		"[$kissue] known bug seems to be fixed"
	    n_known_fixed=`expr $n_known_fixed + 1`
	    ;;
	esac
    done <<EOF
`grep -E '^(XFAIL|XPASS) ' "$log"`
EOF
    if [ $status -eq 0 ]; then
	result PASS "$id" "$name"
    elif [ $status -eq 1 ] && [ $n_known_fixed -gt 0 ] &&
	    ! grep -q 'FAILED:' "$log"; then
	# only fixed known bugs (already reported as XPASS)
	result PASS "$id" "$name"
    else
	result FAIL "$id" "$name" "`describe_status $status`"
	grep -vE '^(XFAIL|XPASS) ' "$log" > "$log.failed"
	show_log "$log.failed"
    fi
done

#-- run tests ------------------------------------------------------------------

# verify_ir SRC DIR LOG LIMIT: emit LLVM IR for SRC and check it with the LLVM
# verifier; prints an error message on failure
verify_ir()
{
    vbase=`basename "$1" .abc`
    TMPDIR=$2 run_limited $4 "$ABC" $abc_flags --emit-llvm \
	"$1" -o "$2/$vbase.ll" >> "$3" 2>&1
    vstatus=$?
    if [ $vstatus -ne 0 ]; then
	echo "compiler failed to emit LLVM IR (`describe_status $vstatus`)"
    elif [ -n "$opt" ] && ! "$opt" -passes=verify -disable-output \
	    "$2/$vbase.ll" >> "$3" 2>&1; then
	echo "generated LLVM IR is invalid"
    fi
}

# run_test FILE: compile, verify and execute test program FILE
run_test()
{
    src=$1
    base=`basename "$src" .abc`
    dir=$work/run/$base
    log=$dir/log
    mkdir -p "$dir"
    : > "$log"

    expected_exit=`directive "$src" EXIT`
    expected_exit=${expected_exit:-0}
    limit=`directive "$src" TIMEOUT`
    limit=${limit:-$default_timeout}
    # additional options (ARGS, %b: build directory) follow the input file
    args=`directive "$src" ARGS | sed "s|%b|$ABC_BUILD|g"`
    expected_out=`dirname "$src"`/$base.out
    c_src=`dirname "$src"`/$base.c

    extra_obj=
    if [ -f "$c_src" ]; then
	if ! cc -c -o "$dir/$base-c.o" "$c_src" >> "$log" 2>&1; then
	    echo "compiling $c_src failed"
	    return
	fi
	extra_obj=$dir/$base-c.o
    fi

    msg=`verify_ir "$src" "$dir" "$log" $limit`
    if [ -n "$msg" ]; then
	echo "$msg"
	return
    fi

    for level in $ABC_TEST_OPT; do
	exe=$dir/$base$level
	TMPDIR=$dir run_limited $limit "$ABC" $abc_flags $level \
	    -L$ABC_BUILD "$src" $args $extra_obj -o "$exe" >> "$log" 2>&1
	status=$?
	if [ $status -ne 0 ]; then
	    echo "compilation with $level failed (`describe_status $status`)"
	    return
	fi
	(cd "$dir" && run_limited $limit "$exe" < /dev/null \
	    > "$exe.stdout" 2>> "$log")
	status=$?
	if [ $status -ne $expected_exit ]; then
	    echo "program compiled with $level: `describe_status $status`," \
		"expected $expected_exit"
	    return
	fi
	if [ -f "$expected_out" ] && ! cmp -s "$expected_out" "$exe.stdout"
	then
	    diff -u "$expected_out" "$exe.stdout" >> "$log" 2>&1
	    echo "program compiled with $level: unexpected output"
	    return
	fi
    done
}

for src in "$top"/test/run/*.abc; do
    [ -f "$src" ] || continue
    name=run/`basename "$src"`
    id=`directive "$src" TEST`
    selected "$name" "$id" || continue
    req=`directive "$src" REQUIRES`
    if [ -n "$req" ] && [ "$req" != "$arch" ]; then
	result SKIP "$id" "$name" "(requires $req)"
	continue
    fi
    msg=`run_test "$src"`
    report "$id" "$name" "`issue "$src" XFAIL`" "$msg" \
	"$work/run/`basename "$src" .abc`/log"
done

#-- fail tests -----------------------------------------------------------------

# fail_test FILE: FILE must be rejected with a matching diagnostic
fail_test()
{
    src=$1
    base=`basename "$src" .abc`
    dir=$work/fail/$base
    log=$dir/log
    mkdir -p "$dir"

    pattern=`directive "$src" ERROR`
    limit=`directive "$src" TIMEOUT`
    limit=${limit:-$default_timeout}
    if [ -z "$pattern" ]; then
	echo "test has no '// ERROR:' directive"
	return
    fi

    # additional options (ARGS) follow the input file on purpose
    TMPDIR=$dir run_limited $limit "$ABC" $abc_flags -c "$src" \
	-o "$dir/$base.o" `directive "$src" ARGS | sed "s|%b|$ABC_BUILD|g"` \
	> "$log" 2>&1
    status=$?
    if [ $status -eq 0 ]; then
	echo "compiler accepted invalid program"
    elif [ $status -ge 128 ]; then
	echo "compiler crashed or hung (`describe_status $status`)"
    elif ! strip_color < "$log" | grep -Eiq -e "$pattern"; then
	echo "diagnostic does not match /$pattern/"
    fi
}

for src in "$top"/test/fail/*.abc; do
    [ -f "$src" ] || continue
    name=fail/`basename "$src"`
    id=`directive "$src" TEST`
    selected "$name" "$id" || continue
    msg=`fail_test "$src"`
    report "$id" "$name" "`issue "$src" XFAIL`" "$msg" \
	"$work/fail/`basename "$src" .abc`/log"
done

#-- design tests ---------------------------------------------------------------

# design_test FILE: prints "ACCEPTED", "REJECTED" or an error message
design_test()
{
    src=$1
    base=`basename "$src" .abc`
    dir=$work/design/$base
    log=$dir/log
    mkdir -p "$dir"
    : > "$log"
    limit=`directive "$src" TIMEOUT`
    limit=${limit:-$default_timeout}

    TMPDIR=$dir run_limited $limit "$ABC" $abc_flags -c "$src" \
	-o "$dir/$base.o" >> "$log" 2>&1
    status=$?
    if [ $status -ge 128 ]; then
	echo "CRASH: compiler crashed or hung (`describe_status $status`)"
	return
    elif [ $status -ne 0 ]; then
	echo "REJECTED"
	return
    fi

    # accepted: the generated code must be valid and the program must run
    msg=`verify_ir "$src" "$dir" "$log" $limit`
    if [ -n "$msg" ]; then
	echo "accepted, but $msg"
	return
    fi
    level=`echo $ABC_TEST_OPT | cut -d ' ' -f 1`
    exe=$dir/$base
    TMPDIR=$dir run_limited $limit "$ABC" $abc_flags $level -L$ABC_BUILD \
	"$src" -o "$exe" >> "$log" 2>&1
    status=$?
    if [ $status -ne 0 ]; then
	echo "accepted, but building the program failed" \
	    "(`describe_status $status`)"
	return
    fi
    (cd "$dir" && run_limited $limit "$exe" < /dev/null > "$exe.stdout" \
	2>> "$log")
    status=$?
    if [ $status -ne 0 ]; then
	echo "accepted, but the program failed (`describe_status $status`):" \
	    "observed semantics differ from the test"
	return
    fi
    echo "ACCEPTED"
}

for src in "$top"/test/design/*.abc; do
    [ -f "$src" ] || continue
    name=design/`basename "$src"`
    id=`directive "$src" TEST`
    selected "$name" "$id" || continue
    ref=`issue "$src" DESIGN`
    recorded=`directive "$src" OBSERVED`
    log=$work/design/`basename "$src" .abc`/log
    observed=`design_test "$src"`
    case $observed in
    ACCEPTED|REJECTED)
	if [ "$observed" = "$recorded" ]; then
	    result DESIGN "$id" "$name" "[$ref] $observed"
	else
	    msg="observed $observed, recorded $recorded: update the test"
	    result FAIL "$id" "$name" "[$ref] $msg and issue $ref"
	    show_log "$log"
	fi
	;;
    *)
	result FAIL "$id" "$name" "[$ref] $observed"
	show_log "$log"
	;;
    esac
done

#-- IR tests -------------------------------------------------------------------

# ir_test FILE: compile FILE with the options of its ARGS directive and check
# the generated LLVM IR (prints an error message on failure).  In ARGS, %s
# stands for the source file and %o for the output file; without %s the
# runner appends '--emit-llvm FILE -o OUTPUT'; %b stands for the build
# directory.
ir_test()
{
    src=$1
    base=`basename "$src" .abc`
    dir=$work/ir/$base
    log=$dir/log
    ll=$dir/$base.ll
    mkdir -p "$dir"
    : > "$log"
    limit=`directive "$src" TIMEOUT`
    limit=${limit:-$default_timeout}

    args=`directive "$src" ARGS | sed "s|%b|$ABC_BUILD|g"`
    case $args in
    *%s*) cmd=`echo "$args" | sed -e "s|%s|$src|g" -e "s|%o|$ll|g"` ;;
    *) cmd="$args --emit-llvm $src -o $ll" ;;
    esac
    echo "options: $args" >> "$log"
    # $cmd is split into separate options on purpose
    TMPDIR=$dir run_limited $limit "$ABC" $abc_flags $cmd >> "$log" 2>&1
    status=$?
    if [ $status -ne 0 ]; then
	echo "compiler failed to emit LLVM IR (`describe_status $status`)"
	return
    fi
    if [ -n "$opt" ] && ! "$opt" -passes=verify -disable-output "$ll" \
	    >> "$log" 2>&1; then
	echo "generated LLVM IR is invalid"
	return
    fi
    # each IR-MATCH regex must match a line of the IR
    sed -n 's|^// IR-MATCH: *||p' "$src" > "$dir/match"
    while read -r pattern; do
	if ! grep -Eq -e "$pattern" "$ll"; then
	    echo "IR does not match /$pattern/ (options: $args)"
	    return
	fi
    done < "$dir/match"
    fn=`directive "$src" FUNCTION`
    if [ -n "$fn" ] && ! perl "$top/test/check-ir.pl" "$ll" "$fn" \
	    "`directive "$src" SIZE-ATTRIBUTES`" \
	    "`directive "$src" OPTIMIZED`" > "$dir/check" 2>&1; then
	cat "$dir/check" >> "$log"
	echo "`head -n 1 "$dir/check"` (options: $args)"
    fi
}

for src in "$top"/test/ir/*.abc; do
    [ -f "$src" ] || continue
    name=ir/`basename "$src"`
    id=`directive "$src" TEST`
    selected "$name" "$id" || continue
    msg=`ir_test "$src"`
    report "$id" "$name" "`issue "$src" XFAIL`" "$msg" \
	"$work/ir/`basename "$src" .abc`/log"
done

#-- tool checks ----------------------------------------------------------------

# Scripts in test/tool check the build environment (e.g. the compilation
# database).  Exit status 0: PASS, 77: SKIP (first output line is the
# reason), otherwise FAIL.

for t in "$top"/test/tool/*.pl "$top"/test/tool/*.sh; do
    [ -f "$t" ] || continue
    name=tool/`basename "$t"`
    id=`sed -n 's|^# TEST: *||p' "$t" | head -n 1`
    selected "$name" "$id" || continue
    log=$work/tool-`basename "$t"`.log
    case $t in
    *.pl) interpreter=perl ;;
    *) interpreter=sh ;;
    esac
    TOP=$top LLVM_BINDIR=$LLVM_BINDIR run_limited 300 $interpreter "$t" \
	> "$log" 2>&1
    status=$?
    case $status in
    0) result PASS "$id" "$name" ;;
    77) result SKIP "$id" "$name" "(`head -n 1 "$log"`)" ;;
    *)
	result FAIL "$id" "$name" "`describe_status $status`"
	show_log "$log"
	;;
    esac
done

#-- summary --------------------------------------------------------------------

echo
echo "Summary: $n_pass passed, $n_fail failed, $n_xfail expected failures" \
    "(known bugs), $n_xpass unexpected passes, $n_design open design" \
    "questions, $n_skip skipped"
echo "Known bugs and design questions: test/bugs_and_open_design_questions.md"
if [ $n_fail -ne 0 ] || [ $n_xpass -ne 0 ]; then
    echo "Failing tests:$failed_tests"
    exit 1
fi
exit 0
