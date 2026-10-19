#!/bin/sh
set -eu
[ "$#" = 3 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-q4-phases.XXXXXXXX")
for kind in schedule persistent cooperative; do
 case "$kind" in
  schedule) source=pioneer_q38_q4_phases_test.cpp; extra=;;
  persistent) source=pioneer_q38_q4_persistent_image_test.cpp; extra=-DPIONEER_Q38_Q4_TWO_PHASE;;
  cooperative) source=pioneer_q38_work_cohort_cooperative_test.cpp
   extra='-DPIONEER_Q38_Q4_TWO_PHASE -DPIONEER_Q38_COMPACT_ACTIVATIONS -DPIONEER_Q38_REUSE_ACTIVATIONS -DPIONEER_Q38_WORK_COHORT_CACHE';;
 esac
 ${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
  $extra "$directory/$source" -o "$temporary/$kind"
 case "$kind" in
  schedule) ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$kind";;
  persistent) ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$kind" "$1" "$3";;
  cooperative) ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$kind" "$2" "$1";;
 esac
done
printf 'Offline Q4 integer two-phase evidence retained at %s\n' "$temporary"
