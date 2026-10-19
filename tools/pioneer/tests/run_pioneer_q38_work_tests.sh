#!/bin/sh
set -eu
[ "$#" = 1 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-work.XXXXXXXX")
for payload in 0 1; do
 extra=
 [ "$payload" = 0 ] || extra=-DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_PAYLOAD
 ${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
  -DPIONEER_Q38_COMPACT_ACTIVATIONS -DPIONEER_Q38_REUSE_ACTIVATIONS -DPIONEER_Q38_REUSE_CROSS_QUANT \
  -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR -DTT_PIONEER_Q38_WORK_MODE $extra \
  "$directory/pioneer_q38_work_run_test.cpp" -o "$temporary/work-$payload"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/work-$payload" "$1"
done
${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
 "$directory/pioneer_q38_cpu_validation_test.cpp" -o "$temporary/cpu"
ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/cpu"
for mode in work cpu; do
 extra=
 [ "$mode" = work ] || extra=-DPIONEER_Q38_CPU_VALIDATION
 ${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
  -DTT_PIONEER_EXPERIMENTAL_Q38_COHORT_SEALED -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR $extra \
  "$directory/pioneer_q38_mode_session_test.cpp" -o "$temporary/session-$mode"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/session-$mode"
done
printf 'Offline Work/CPU validation evidence retained at %s\n' "$temporary"
