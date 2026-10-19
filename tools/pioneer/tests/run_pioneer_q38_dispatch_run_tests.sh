#!/bin/sh
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-dispatch-run.XXXXXXXX")
for payload in 0 1; do
for profile in 0 1; do
 extra=
 [ "$payload" = 0 ] || extra=-DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD
 [ "$profile" = 0 ] || extra="$extra -DTT_PIONEER_Q38_DISPATCH_PROFILE"
 ${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined $extra \
  "$directory/pioneer_q38_dispatch_run_test.cpp" -o "$temporary/test-$payload-$profile"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/test-$payload-$profile" "$1"
done
done
printf 'Offline persistent RUN controller evidence retained at %s\n' "$temporary"
