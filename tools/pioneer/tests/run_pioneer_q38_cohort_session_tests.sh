#!/bin/sh
set -eu
[ "$#" = 0 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-cohort-session.XXXXXXXX")
for mode in checked work; do
	flags=
	if [ "$mode" = work ]; then flags=-DPIONEER_Q38_WORK_INFERENCE=1; fi
	${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
	 -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED $flags "$directory/pioneer_q38_cohort_session_test.cpp" -o "$temporary/test-$mode"
	ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test-$mode"
done
printf 'Offline persistent session evidence retained at %s\n' "$temporary"
