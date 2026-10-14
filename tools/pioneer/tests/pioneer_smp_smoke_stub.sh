#!/bin/sh
set -eu
test "$#" -eq 1 && test "$1" = --expect-cpus=64
test -f "$SMP_SMOKE_TEST_BASE/consumed-v1"
test ! -e "$SMP_SMOKE_TEST_BASE/armed"
printf 'mock pinned CPU workload\n'
exit "${SMP_SMOKE_TEST_RESULT:-0}"
