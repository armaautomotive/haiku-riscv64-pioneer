#!/bin/sh
# Profiled ACTUAL owner+frozen ROOT/ALL119 interpreter and faults. NO device.
set -eu
[ "$#" = 3 ] || exit 2
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
sh "$directory/run_pioneer_q38_ffn_input_graph_images_tests.sh" "$1" "$2"
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-q38-ffn-host-profile.XXXXXXXX")
for source in pioneer_q38_ffn_host_phase_profile_test pioneer_q38_ffn_persistent_graph_fault_test pioneer_q38_ffn_persistent_graph_test;do
 ${CXX:-c++} -std=c++17 -O1 -g -ffp-contract=off -Wall -Wextra -Werror -fsanitize=address,undefined \
  -DPIONEER_Q38_FFN_GRAPH_INPUT_ROOT=1 -DPIONEER_Q38_FFN_PERSISTENT_GRAPH=1 -DPIONEER_Q38_GDN_MUTABLE_STATE \
  -DPIONEER_Q38_FFN_HOST_PROFILE=1 \
  -DPIONEER_Q38_TEST_WORK_DOT_ZERO_ELISION -DPIONEER_Q38_TEST_AFFINE_REPLAY \
  -DPIONEER_Q38_TEST_BLOCK_SCALE_REUSE -DPIONEER_Q38_TEST_INT_ACCUMULATOR -DPIONEER_Q38_TEST_PIPELINED_MATRIX \
  "$directory/$source.cpp" -o "$temporary/$source"
 ASAN_OPTIONS=quarantine_size_mb=32 UBSAN_OPTIONS=halt_on_error=1 "$temporary/$source" "$@"
done
printf 'Profiled full-FFN owner evidence: %s;NO native timing or device access\n' "$temporary"
