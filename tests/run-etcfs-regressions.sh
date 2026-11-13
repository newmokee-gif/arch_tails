#!/bin/sh
set -eu
out="${TMPDIR:-/tmp}/bedrock-renameat2-test.$$"
trap 'rm -f "$out"' EXIT HUP INT TERM
cc -std=c99 -Wall -Wextra -Werror tests/test_renameat2.c -o "$out"
set +e
"$out"
status=$?
set -e
case "$status" in
0) echo "renameat2 regression test passed" ;;
77) echo "renameat2 regression test skipped: syscall unavailable" ;;
*) echo "renameat2 regression test failed" >&2; exit "$status" ;;
esac
