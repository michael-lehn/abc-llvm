# TEST: T0189
#
# Optional integration check of the compilation database with clangd (skipped
# if clangd is not available).  Uses the clangd of the selected LLVM
# installation (LLVM_BINDIR), otherwise clangd from PATH.  For each file
# clangd must load TOP/compile_commands.json, use its entry for the file (not
# a fallback command), and report no diagnostics (e.g. missing headers or
# unknown options).  Run by test/run-tests.sh.
#
# Exit status 0: ok; 77: clangd not available; 1: errors.

files="abc/abc.cpp gen/function.cpp parser/parser.cpp test/unit/test_type.cpp"

if [ -z "$TOP" ] || [ ! -f "$TOP/compile_commands.json" ]; then
    echo "requires 'make check' (no compile_commands.json)"
    exit 77
fi
clangd=
if [ -n "$LLVM_BINDIR" ] && [ -x "$LLVM_BINDIR/clangd" ]; then
    clangd=$LLVM_BINDIR/clangd
elif command -v clangd > /dev/null 2>&1; then
    clangd=`command -v clangd`
else
    echo "clangd not found"
    exit 77
fi
"$clangd" --version | head -n 1

cd "$TOP" || exit 1
status=0
for f in $files; do
    log=`"$clangd" --check="$f" --check-locations=false --log=verbose 2>&1`
    errors=
    echo "$log" | grep -qF "Loaded compilation database from $TOP/compile_commands.json" ||
	errors="$errors; compilation database not loaded"
    echo "$log" | grep -q 'Compile command from CDB is' ||
	errors="$errors; no compile command from the database (fallback)"
    echo "$log" | grep -q 'All checks completed' ||
	errors="$errors; clangd did not complete"
    # diagnostics look like 'E[time] [pp_file_not_found] Line 11: ...'
    diags=`echo "$log" | grep -E '^E\[[^]]*\] \[[A-Za-z_]+\]'`
    [ -n "$diags" ] && errors="$errors; diagnostics:
$diags"
    if [ -n "$errors" ]; then
	echo "$f: FAILED${errors#;}"
	status=1
    else
	echo "$f: ok (`echo "$log" | grep -c 'Compile command from CDB'` database entry)"
    fi
done
exit $status
