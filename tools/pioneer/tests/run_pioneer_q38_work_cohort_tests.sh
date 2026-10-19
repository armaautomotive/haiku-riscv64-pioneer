#!/bin/sh
set -eu
[ "$#" = 2 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-work-cohort.XXXXXXXX")
for kind in image cooperative; do
 ${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
  -DPIONEER_Q38_COMPACT_ACTIVATIONS -DPIONEER_Q38_REUSE_ACTIVATIONS -DPIONEER_Q38_WORK_COHORT_CACHE \
  "$directory/pioneer_q38_work_cohort_${kind}_test.cpp" -o "$temporary/$kind"
 if [ "$kind" = image ]; then
  ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$kind" "$1"
 else
  ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$kind" "$1" "$2"
 fi
done
printf 'Offline Work cohort instruction evidence retained at %s\n' "$temporary"
