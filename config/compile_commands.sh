#!/bin/sh
#
# Writes a Clang compilation database (compile_commands.json) for the C++
# sources of the compiler.  Called by the Makefile target 'compile_cmd':
#
#   sh config/compile_commands.sh OUTPUT BUILD_DIR SOURCE... -- COMPILER ARG...
#
# For each SOURCE the entry records the command used by the Makefile:
#
#   COMPILER ARG... -I. -o BUILD_DIR/SOURCE.o -MT BUILD_DIR/SOURCE.o -MMD -MP SOURCE
#
# with SOURCE.o meaning SOURCE with '.cpp' replaced by '.o'.  The directory of
# all entries is the current working directory.  Arguments are stored as
# "arguments" arrays, so no shell quoting is involved.
#

if [ $# -lt 4 ]; then
    echo "usage: $0 OUTPUT BUILD_DIR SOURCE... -- COMPILER ARG..." >&2
    exit 2
fi

output=$1
build_dir=$2
shift 2

set -f

sources=
while [ $# -gt 0 ] && [ "$1" != "--" ]; do
    sources="$sources $1"
    shift
done
if [ "$1" != "--" ] || [ $# -lt 2 ]; then
    echo "$0: missing '-- COMPILER ARG...'" >&2
    exit 2
fi
shift

# json STRING: print STRING as JSON string literal (escapes backslash, double
# quote, tab, carriage return and newline)
json()
{
    printf '%s' "$1" | awk '
	BEGIN { tab = sprintf("\t"); cr = sprintf("\r") }
	{
	    gsub(/\\/, "\\\\")
	    gsub(/"/, "\\\"")
	    gsub(tab, "\\t")
	    gsub(cr, "\\r")
	    line[NR] = $0
	}
	END {
	    printf "\""
	    for (i = 1; i <= NR; ++i) {
		printf "%s%s", (i > 1 ? "\\n" : ""), line[i]
	    }
	    printf "\""
	}'
}

directory=`pwd`

# the compiler and its arguments are the same for all entries
common=
for arg in "$@"; do
    common="$common`json "$arg"`, "
done
common="$common`json -I.`"

tmp=$output.tmp
{
    echo "["
    sep=
    for src in $sources; do
	obj=$build_dir${src%.cpp}.o
	printf '%s  {\n' "$sep"
	printf '    "directory": %s,\n' "`json "$directory"`"
	printf '    "file": %s,\n' "`json "$directory/$src"`"
	printf '    "output": %s,\n' "`json "$obj"`"
	printf '    "arguments": [%s, "-o", %s, "-MT", %s, "-MMD", "-MP", %s]\n' \
	    "$common" "`json "$obj"`" "`json "$obj"`" "`json "$src"`"
	printf '  }'
	sep=",
"
    done
    echo
    echo "]"
} > "$tmp" && mv "$tmp" "$output"
