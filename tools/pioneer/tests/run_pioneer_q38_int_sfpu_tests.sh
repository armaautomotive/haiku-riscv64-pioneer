#!/bin/sh
# OFFLINE ONLY: actual emitted peer plus unchanged root, NOT native launch.
set -eu
[ "$#" = 3 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
peer=$1; root=$2; model=$3
extra="${CXX:-c++} -DPIONEER_Q38_TEST_INT_ACCUMULATOR -DPIONEER_Q38_INT_SFPU -DTT_PIONEER_EXPERIMENTAL_Q38_DISPATCH_VECTOR -DPIONEER_Q38_Q4_TWO_PHASE"
sh "$directory/run_pioneer_q38_sfpu_accumulator_tests.sh"
CXX="$extra" sh "$directory/run_pioneer_q38_vector_persistent_image_tests.sh" "$peer" "$model"
CXX="$extra" sh "$directory/run_pioneer_q38_work_cohort_tests.sh" "$root" "$peer"
CXX="$extra -DPIONEER_Q38_COMPACT_ACTIVATIONS -DPIONEER_Q38_REUSE_ACTIVATIONS -DPIONEER_Q38_REUSE_CROSS_QUANT -DTT_PIONEER_Q38_WORK_MODE -DPIONEER_Q38_WORK_POSTED_STAGE -DPIONEER_Q38_WORK_COHORT_CACHE -DPIONEER_Q38_SPAN_PRELOAD -DPIONEER_Q38_SPAN_FAST -DPIONEER_Q38_SPAN_REUSE_CODE" \
 sh "$directory/run_pioneer_q38_dispatch_control_tests.sh" "$root"
printf 'Offline integer/SFPU protocol/peer/cooperative/owner evidence complete; NO native qualification.\n'
