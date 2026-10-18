#!/bin/sh
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-cohort-client.XXXXXXXX")
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -I"$directory/cli-stubs" "$directory/pioneer_q38_cohort_client_test.cpp" -o "$temporary/test"
if ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test" "$1" > "$temporary/test.log" 2>&1; then
 tail -n 1 "$temporary/test.log"
else
 tail -n 40 "$temporary/test.log"
 exit 1
fi
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED -I"$directory/cli-stubs" "$directory/pioneer_q38_cohort_client_test.cpp" -o "$temporary/sealed"
if ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/sealed" "$1" > "$temporary/sealed.log" 2>&1; then
 tail -n 1 "$temporary/sealed.log"
else
 tail -n 40 "$temporary/sealed.log"
 exit 1
fi
printf 'Offline cohort real-client evidence retained at %s\n' "$temporary"
